from multiprocessing import synchronize, Lock
import os
import threading
import time
import datetime
import uuid
import weakref
import MobileListener

try:
    from xmlrpclib import Fault
    from xmlrpclib import ServerProxy
except ImportError:
    from xmlrpc.client import Fault
    from xmlrpc.client import ServerProxy


class RuntimeException(Exception):
    def __init__(self, desc, errObj):
        self.description = desc
        self.errObj = errObj
    def __str__(self):
        return repr(self.description) + " : \n" + repr(self.errObj)

class InternalException(RuntimeException):
    def __init__(self, cause, message, causeId):
        RuntimeException.__init__(self, message, cause)
        self.cause = causeId
        
class Configuration:
    def __init__(self):
        self.host = "127.0.0.1"
        self.port = 8888
        self.projectBaseDirectory = None
        self.deviceName = None
        self.clientMonitor = None
        self.findAgentPort = False
        self.agentIdleTime = 60000
        self.reportsDirectory = "reports"
        self.testName = None
        self.envVarOverwrite = True
    
    def getHost(self):
        return self.getProperty("host", self.host, self.envVarOverwrite)
   
    def setHost(self, host):
        self.host = host
            
    def getPort(self):
        return int(self.getProperty("port", self.port, self.envVarOverwrite))

    def setPort(self, port):
        self.port = port;
     
    def getProjectBaseDirectory(self):
        return self.getProperty("projectBaseDirectory", self.projectBaseDirectory, self.envVarOverwrite)

    def setProjectBaseDirectory(self, projectBaseDirectory):
        self.projectBaseDirectory = projectBaseDirectory

    def getDevice(self):
        return self.getProperty("deviceName", self.deviceName, self.envVarOverwrite)

    def setDevice(self, deviceName):
        self.deviceName = deviceName

    def isFindAgentPort(self):
        return (self.getProperty("findAgentPort", self.findAgentPort, self.envVarOverwrite) in ['true','True',True])

    def setFindAgentPort(self, findAgentPort):
        self.findAgentPort = findAgentPort

    def getAgentIdleTime(self):
        return self.getProperty("agentIdleTime", self.agentIdleTime, self.envVarOverwrite)

    def setAgentIdleTime(self, agentIdleTime):
        self.agentIdleTime = agentIdleTime
            
    def getReportsDirectory(self):
        return self.getProperty("reportsDirectory", self.reportsDirectory, self.envVarOverwrite)

    def setReportsDirectory(self, reportsDirectory):
        self.reportsDirectory = reportsDirectory

    def getTestName(self):
        return self.getProperty("testName", self.testName, self.envVarOverwrite)

    def setTestName(self, testName):
        self.testName = testName;

    def isEnviromentVariableOverwrite(self):
        return self.envVarOverwrite;

    def setEnviromentVariableOverwrite(self, envVarOverwrite):
        self.envVarOverwrite = envVarOverwrite

    def getProperty(self, property, default, envVarOverwrite):
        value = default
        if (envVarOverwrite==True):
            try:
                value = os.environ[property]
            except:
                   value = default
        return value;

class KeepAlive (threading.Thread):
    _shouldStop = False
    
    def __init__(self, clientID, clientref):
        threading.Thread.__init__(self)
        #self.setDaemon(True)
        
        self.clientID = clientID
        self.clientWeakRef = clientref
    def run(self):
        try :
            while ((not self.clientWeakRef() is None) and (not self._shouldStop)):
                #print "Starting " + self.clientID
                (self.clientWeakRef()).pingServer()
                time.sleep(1)
            #print "Exiting " + self.clientID
        finally:
            self._shouldStop = True
            print("")
            
    def release(self):
        self._shouldStop = True
    
    def isRunning(self):
        return not self._shouldStop

class Client:
    """An object of class Client controls an agent, which is part of the SeeTest.
       The client and the agent can run on either the same machine or different machines.
       It uses XML-RPC calls to communicate with the agent."""

    _host = ""
    debug = False
    def oldInit(self, host="127.0.0.1", port=8889):
        self.throwExceptionOnFail = True
        url = "http://" + host + ":" + repr(port) + "/xmlrpc"
        self.client = ServerProxy(url)
        _host = host
        self.clientID = None
        print(url)
        
  
    def init(self, host="127.0.0.1", port=8889, useSessionID = False):
        """Instantiates a new client. The host and port can be used to connect to a remote machine.
           If it runs on the local machine, then '127.0.0.1' and 8889 should be passed.
           The agent port can be controlled through the user interface of SeeTest.
           Args:
               host (str, '127.0.0.1'): host name or IP address of agent
               port (int, 8889): port of agent"""
        #client = Client()
        self.oldInit(host, port)
        self.lock = Lock()
        self.clientID = None
        self.listeners = dict()
        if(useSessionID):
            self.clientID = "clientID:Python:version=" + self.getVersion() + ":" + str(uuid.uuid1())
            self.clientMonitor = KeepAlive(self.clientID, weakref.ref(self))
            (self.clientMonitor).start()
        

    def __del__(self):
        pass

    def testing(self):
        print("Testing, 1.2.3")
        
    @staticmethod   
    def configure(config):
        client = Client()
        client.init(config.getHost(),config.getPort())
        if(config.isFindAgentPort()):
            port = client.getAvilableAgentPort(config.getAgentIdleTime())
            client.init(config.getHost(), port)
        if not(config.getProjectBaseDirectory() is None):
            client.setProjectBaseDirectory(config.getProjectBaseDirectory())
        if (config.getTestName() is None):
            client.setReporter("xml", config.getReportsDirectory())
        else:
            client.setReporter2("xml", config.getReportsDirectory(), config.getTestName())
        if not(config.getDevice() is None):
            client.setApplicationTitle(config.getDevice())
        return client
    
    def setClientDebugStatus(self, newStatus):
        self.debug = newStatus


    def __executeXmlRpcCommand(self, methodName, args):
        if (self.debug == True) :
            print(datetime.datetime.now().strftime("%H:%M:%S:%f") + ":" + self.clientID + " Method: " + methodName + " args: " + "[" + ", ".join( str(x) for x in args) + "]")
        o = getattr(self.client, "agent." + methodName)(*args)
        if (self.debug == True) :
            print(datetime.datetime.now().strftime("%H:%M:%S:%f") + ":" + self.clientID + " Method: " + methodName + " Result: " + str(o))
        return o


    def __executer(self, methodName, args):
        """Executer
           Args:
               methodName (str): the invoked method name
               args ([Object]): the arguments
           Returns:
               dict of (str, Object): a mapping of resulted values
               The following keys can be found in the map:
               {
                 status (boolean): indicate the last command status;
                 logLine (str): summaries the last command;
                 errorMessage (str): is available only in case of false 'status' with error indication;
                 exception (str): is available only in case of false 'status' with exception stack trace (optional);
                 outFile (str): with the path to an image of the report;
                 position : element position;
                 center : element center;
                 time : operation time.
               }"""
        try:
            if(len(args) > 12):
                raise InternalException(None, "Unsupported number of arguments", "USER_INPUT_ERROR")
                return None     
            else:   
                if(not (self.clientID is None)):
                    if(not(self.clientMonitor is None) and not (self.clientMonitor.isRunning()) and not methodName is "pingServer"):
                        print("The client has already been released. Please create a new client")
                    a = list(args)
                    a.insert(0, self.clientID)
                    args = tuple(a)
                o = self.__executeXmlRpcCommand(methodName, args)
        except Fault as err:
            print("A fault occurred")
            print("Fault code: %d" % err.faultCode)
            print("Fault string: %s" % err.faultString)
            raise InternalException(err, "Connectivity Exception, verify studio is open and the right port is been used", "INTERNAL_ERROR");

        if(type(o) is Exception):
            raise RuntimeException(methodName, o)
        if (type(o) is dict):
            map_ = o
            
            if ("xpathFound" in map_):
                typeXpathObj = map_["xpathFound"]
                currentListener =  self.listeners[typeXpathObj];
                if(currentListener is None) :
                    print("XPath event was triggered: " + typeXpathObj + ", listener wasn't found")
                else :
                    delemiterIndex = typeXpathObj.index(':')
                    if delemiterIndex >= 0 :
                        try :
                            currentListener.setClient(self)
                            self.lock.release()
                            continueExe = currentListener.recover(typeXpathObj[0:delemiterIndex], typeXpathObj[delemiterIndex + 1:])
                            if continueExe :
                                return self.__executer(methodName, args)
                        finally:
                            self.lock.acquire()
            if (not methodName is "pingServer"):
                self.map = o
                print(methodName)
            if (map_['status'] == False):
                if ((self.throwExceptionOnFail == True) or ('throwException' in map_ and map_['throwException'] == True)):
                    print("Status: False")
                    if ('cause' in map_):
                        causeId = map_['cause']
                    else:
                        causeId = "UNKNOWN"
                    raise InternalException(None, "Exception caught while executing " + methodName + ": " + map_['errorMessage'], causeId)
            if('clientReleased' in map_ and map_['clientReleased'] == True):
                self.clientMonitor.release()
            if('logLine' in map_ and map_['logLine'] != "ping server"):
                try:
                    print(map_['logLine'])
                except:
                    pass
            if('text' in map_):
                map_['text'] = map_['text'].encode('utf8')
                try:
                    print(map_['text'])
                except:
                    pass
            return map_
        elif (type(o) is str):
            print(methodName)
            if (methodName is "pingServer"):
                #print methodName
                self.map = {'text':o}
            map_ = {'text':o}
            return map_
        else:
            return None

    def execute(self, methodName, *args):
        try:
            self.lock.acquire()
            ret = self.__executer(methodName, args)
            self.lock.release()
            return ret
        except UnicodeEncodeError as err:
            self.lock.release()
            print("cannot show non Unicode characters")
            return {"text":"cannot show non Unicode characters"}
        except Exception as err:
            #assert methodName + " fails"
            self.lock.release()
            raise err
        except:
            self.lock.release()
            raise
        
    def addMobileListener(self, elementType, xpath, mobileListner):
        if(mobileListner is None):
            self.execute("removeXPathListener", elementType, xpath)
            del self.listeners[elementType + ":" + xpath]
        elif (isinstance(mobileListner, MobileListener.MobileListener)):
            self.execute("addXPathListener", elementType, xpath)
            key = elementType + ":" + xpath
            self.listeners[key] =  mobileListner;
        else:
            raise InternalException(None, "Unexpected mobileListener type", "USER_INPUT_ERROR")


    def pingServer(self):
        resultMap = self.execute("pingServer")
        if (self.isThrowExceptionOnFail()):
            if (resultMap.get("errorMassage")):
                assert resultMap.get("errorMassage")
                
                
    def getLastCommandResultMap(self):
        """Gets the last command result map. It can be used to integrate the SeeTest reports to external reports system.
           Returns:
               dict of (str, Object): a mapping of resulted values
               The following keys can be found in the map:
               {
                 status (boolean): indicate the last command status;
                 logLine (str): summaries the last command;
                 errorMessage (str): is available only in case of false 'status' with error indication;
                 exception (str): is available only in case of false 'status' with exception stack trace (optional);
                 outFile (str): with the path to an image of the report;
                 position : element position;
                 center : element center;
                 time : operation time.
               }"""           
        return self.map;

    def isThrowExceptionOnFail(self):
        """Checks whether an exception is thrown on a failure.
           Returns:
               true if and only if an exception is thrown upon failure"""
        return self.throwExceptionOnFail;

    def setThrowExceptionOnFail(self, _throwExceptionOnFail):
        """Sets whether to throw an exception upon a failure.
           Args:
               _throwExceptionOnFail (boolean): the flag whether to throw an exception upon a failure"""
        self.throwExceptionOnFail = _throwExceptionOnFail;
    
    def getClientConfigurationFromUI(self):
        arrConfig=self.execute("getClientConfigurationFromUI").get("textArray")
        return arrConfig

    def waitForDevice(self, quary, timeout): #timeout is given in ms
        """
        * Description: Command for the Executor Add-On. This command allows you to wait for a device with specific properties, and then execute the test on the device once it's available. The query is build using XPath syntax. For available properties of a specific device, use the GetDeviceInformation command. In order to release the device and make it available for other future tests, use the Release command.
        *
        * @param  Search device query.
        * @param  Timeout in milliseconds to wait for an available device according to the search query.
        *
        * This command acts as follows:
        * 1. Gets a list of all devices that match the user's query.
        * 2. Filters out devices that are not available (reserved/locked/off-line).
        * 3. Filters out devices that have already been locked for the current client.
        * 4. Sorts the list (preferring local devices over remote ones). 
        * 5. Iterates over the devices list, and over the available agents, until a match is found.
        * 6. If a match is found, the device is locked and set active, the previous used agent (if any) turns free, and the new one is locked. 
        * 7. The following applies only in the first call for a specific client: if the current agent does not match the required license, the command tries to lock another agent.
        * 8. If no match is found or a timeout has passed, an exception is thrown.
        * The command returns the name of the newly locked device.
        ** If the device is already locked for the client - it will not be set again. The command will look for an other device.
        *
        * @return device name if and only if command succeeded; otherwise an exception is thrown.
        """

        timeouttime = time.time() + timeout/1000.0
        oldThrowExceptionOnFail = self.throwExceptionOnFail
        mmap = dict()
        try :
            self.throwExceptionOnFail = False
            while True:
                if time.time() > timeouttime:
                    assert "Timeout wait for device"
                    self.report("waitForDevice - Timeout", False);
                    raise InternalException(None, "Timeout wait for device", "OPERATION_FAILURE");
                try:
                    mmap = self.execute("lockDevice", quary);
                except ValueError as err:
                    print("A fault occurred")
                    print("Fault code: %d" % err.faultCode)
                    print("Fault string: %s" % err.faultString)
                if "status" in mmap and mmap["status"]:
                    dname = mmap["name"]
                    self.report("wait For " + quary, True);
                    return dname
                else:
                    assert "LockDevice fails"
                
                if "validXPath" in mmap and not mmap["validXPath"]:
                    assert "XPath is Invalid"
                    self.report("waitForDevice - XPath is invalid", False);
                    if oldThrowExceptionOnFail :
                        raise InternalException(None, "XPath is invalid", "USER_INPUT_ERROR");
                    return None
                if "license" in mmap and not mmap["license"]:
                    assert "License is not supported on this agent"
                    self.report("waitForDevice - License is not supported on this agent", False);
                    if oldThrowExceptionOnFail :
                        raise InternalException(None, "License is not supported on this agent", "OPERATION_FAILURE");
                    return None
                time.sleep(3)
        finally :
            self.throwExceptionOnFail = oldThrowExceptionOnFail

                
    def getVersion(self): return "11.3"

    def activateVoiceAssistance(self, text):
        """Activates the voice assistant service (e.g. Siri) on the device, and sends the text argument as if it was a spoken directive.
        Args:
                text (str): Text to send"""
        self.execute("activateVoiceAssistance", text)

    def addDevice(self, serialNumber, deviceName):
        """Add / reserve device. return the name that should be used to access the device.
        Args:
                serialNumber (str): the device serial number / UDID
                deviceName (str): the device suggested name (will add an index if the name exists)"""
        return self.execute("addDevice", serialNumber, deviceName).get("name")

    def applicationClearData(self, packageName):
        """Clear application data
        Args:
                packageName (str): The application's package name"""
        self.execute("applicationClearData", packageName)

    def applicationClose(self, packageName):
        """Close application
        Args:
                packageName (str): The application's package name
        Returns:
                boolean: successful close"""
        return self.execute("applicationClose", packageName).get("found")

    def capture(self):
        """Capture the current screen and add it to the report.
        Returns:
                str: the path of the captured image file."""
        return self.execute("capture").get("outFile")

    def captureLine(self, line="Capture"):
        """Capture the current screen and add it to the report with the given line.
        Args:
                line (str, 'Capture'): the line to be used in the report
        Returns:
                str: the path of the captured image file."""
        return self.execute("capture", line).get("outFile")

    def captureElement(self, name, x, y, width, height, similarity=97):
        """Create new element from image in given device coordinates, with given name and similarity percentage.
        Args:
                name (str): New element's name
                x (int): Image origin x coordinate
                y (int): Image origin y coordinate
                width (int): Image width
                height (int): Image height
                similarity (int, 97): the similarity between pictures"""
        self.execute("captureElement", name, x, y, width, height, similarity)

    def clearAllSms(self):
        """Clear all SMS on a device."""
        self.execute("clearAllSms")

    def clearDeviceLog(self):
        """Clear device log"""
        self.execute("clearDeviceLog")

    def clearLocation(self):
        """Clears the mock location. Currently supported only on android"""
        self.execute("clearLocation")

    def click(self, zone, element, index=0, clickCount=1):
        """Click an element.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                clickCount (int, 1): Number of Clicks"""
        self.execute("click", zone, element, index, clickCount)

    def clickOffset(self, zone, element, index=0, clickCount=1, x=0, y=0):
        """Click on or near to an element. The offset is specified by x, y.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                clickCount (int, 1): Number of Clicks
                x (int, 0): Horizontal Offset from Element
                y (int, 0): Vertical Offset from Element"""
        self.execute("click", zone, element, index, clickCount, x, y)

    def clickCoordinate(self, x=0, y=0, clickCount=1):
        """Click in window X,Y coordinates related to the device screen.
        Args:
                x (int, 0): Horizontal coordinate
                y (int, 0): Vertical coordinate
                clickCount (int, 1): Number of clicks"""
        self.execute("clickCoordinate", x, y, clickCount)

    def clickIn(self, zone, searchElement, index, direction, clickElement, width=0, height=0):
        """Search for an element and click on an element near him. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                clickElement (str): Click Element
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        self.execute("clickIn", zone, searchElement, index, direction, clickElement, width, height)

    def clickIn2(self, zone, searchElement, index, direction, clickElementZone, clickElement, width=0, height=0):
        """Search for an element and click on an element near him. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                clickElementZone (str): Click Element Zone
                clickElement (str): Click Element
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        self.execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, width, height)

    def clickIn2_5(self, zone, searchElement, index, direction, clickElementZone, clickElement, width=0, height=0, clickCount=1):
        """Search for an element and click on an element near him. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                clickElementZone (str): Click Element Zone
                clickElement (str): Click Element
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)
                clickCount (int, 1): Number of Clicks"""
        self.execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, width, height, clickCount)

    def clickIn3(self, zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex=0, width=0, height=0, clickCount=1):
        """Search for an element and click on an element near him. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                clickElementZone (str): Click Element Zone
                clickElement (str): Click Element
                clickElementIndex (int, 0): Click Element Index
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)
                clickCount (int, 1): Number of Clicks"""
        self.execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex, width, height, clickCount)

    def clickTableCell(self, zone, headerElement, headerIndex, rowElement, rowIndex=0):
        """Clicking on table cell by its header element and row element.
        Args:
                zone (str): Select zone
                headerElement (str): Select table header element
                headerIndex (int, 0): Header element index
                rowElement (str): Select table row element
                rowIndex (int, 0): Row element index"""
        self.execute("clickTableCell", zone, headerElement, headerIndex, rowElement, rowIndex)

    def closeDevice(self):
        """Close connection to the device."""
        self.execute("closeDevice")

    def closeDeviceReflection(self):
        """Hides the current device screen. If the current device screen is not open, this command does nothing."""
        self.execute("closeDeviceReflection")

    def closeKeyboard(self):
        """Close device keyboard"""
        self.execute("closeKeyboard")

    def collectSupportData(self, zipDestination, applicationPath, device, scenario, expectedResult, actualResult):
        return self.execute("collectSupportData", zipDestination, applicationPath, device, scenario, expectedResult, actualResult).get("source")

    def collectSupportData2(self, zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData=True, onlyLatestLogs=True):
        return self.execute("collectSupportData", zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData, onlyLatestLogs).get("source")

    def deviceAction(self, action):
        """Execute any of the following device actions:
			Home, Back, Power, Landscape, Portrait, Change Orientation, Menu, Unlock, Paste, Volume Up, Volume Down, Recent Apps.
        Args:
                action (str): Name of action to perform"""
        self.execute("deviceAction", action)

    def drag(self, zone, element, index=0, xOffset=0, yOffset=0):
        """Drag an element in a specified zone.
        Args:
                zone (str): Select Zone
                element (str): Select Element to drag
                index (int, 0): Element Order (=the number of times the element appears more and above the first time)
                xOffset (int, 0): X drag offset
                yOffset (int, 0): Y drag offset"""
        self.execute("drag", zone, element, index, xOffset, yOffset)

    def dragCoordinates(self, x1=0, y1=0, x2=0, y2=0):
        """Drag base on coordinates
        Args:
                x1 (int, 0): First point X
                y1 (int, 0): First point Y
                x2 (int, 0): Second point X
                y2 (int, 0): Second point Y"""
        self.execute("dragCoordinates", x1, y1, x2, y2)

    def dragCoordinates2(self, x1=0, y1=0, x2=0, y2=0, time=2000):
        """Drag base on coordinates
        Args:
                x1 (int, 0): First point X
                y1 (int, 0): First point Y
                x2 (int, 0): Second point X
                y2 (int, 0): Second point Y
                time (int, 2000): Drag time (ms)"""
        self.execute("dragCoordinates", x1, y1, x2, y2, time)

    def dragDrop(self, zone, dragElement, dragIndex, dropElement, dropIndex=0):
        """Drag an element in a specified zone and drop it at a second element
        Args:
                zone (str): Select Zone
                dragElement (str): Drag element
                dragIndex (int, 0): Drag element index
                dropElement (str): Drop element
                dropIndex (int, 0): Drop element index"""
        self.execute("dragDrop", zone, dragElement, dragIndex, dropElement, dropIndex)

    def drop(self):
        """Drop all project information."""
        self.execute("drop")

    def elementGetProperty(self, zone, element, index, property):
        """Get element property
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                property (str): Property"""
        return self.execute("elementGetProperty", zone, element, index, property).get("text")

    def elementGetTableRowsCount2(self, tableLocator, tableIndex=0, visible=False):
        """Get table total or visible rows count
        Args:
                tableLocator (str): Table Locator
                tableIndex (int, 0): Table Locator Index
                visible (boolean, false): Only visible"""
        return self.execute("elementGetTableRowsCount", tableLocator, tableIndex, visible).get("count")

    def elementGetTableRowsCount(self, zone, tableLocator, tableIndex=0, visible=False):
        """Get table total or visible rows count
        Args:
                zone (str): Select Zone
                tableLocator (str): Select Table Locator
                tableIndex (int, 0): Table Locator Index
                visible (boolean, false): Only visible"""
        return self.execute("elementGetTableRowsCount", zone, tableLocator, tableIndex, visible).get("count")

    def elementGetTableValue(self, rowLocator, rowLocatorIndex, columnLocator):
        """Get table cell value
        Args:
                rowLocator (str): Row Locator
                rowLocatorIndex (int, 0): Row Locator Index
                columnLocator (str): Column Locator"""
        return self.execute("elementGetTableValue", rowLocator, rowLocatorIndex, columnLocator).get("text")

    def elementGetText(self, zone, element, index=0):
        """Get text from element
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index"""
        return self.execute("elementGetText", zone, element, index).get("text")

    def elementListPick(self, listZone, listLocator, elementZone, elementLocator, index=0, click=True):
        """Select an element in a list (first make the element visible)
        Args:
                listZone (str): Select List Zone
                listLocator (str): List locator
                elementZone (str): Select Element Zone
                elementLocator (str): Element locator
                index (int, 0): Element index
                click (boolean, true): If TRUE then click"""
        self.execute("elementListPick", listZone, listLocator, elementZone, elementLocator, index, click)

    def elementListSelect(self, listLocator, elementLocator, index=0, click=True):
        """Select an element in a list (first make the element visible)
        Args:
                listLocator (str): List locator
                elementLocator (str): Element locator
                index (int, 0): Element index
                click (boolean, true): If TRUE then click"""
        self.execute("elementListSelect", listLocator, elementLocator, index, click)

    def elementListVisible(self, listLocator, elementLocator, index=0):
        """Make the target element visible
        Args:
                listLocator (str): List locator
                elementLocator (str): Element locator
                index (int, 0): Element index"""
        return self.execute("elementListVisible", listLocator, elementLocator, index).get("found")

    def elementScrollToTableRow2(self, tableLocator, tableIndex=0, rowIndex=0):
        """Scroll table / list to the given row
        Args:
                tableLocator (str): Table Locator
                tableIndex (int, 0): Table Locator Index
                rowIndex (int, 0): Row Index"""
        self.execute("elementScrollToTableRow", tableLocator, tableIndex, rowIndex)

    def elementScrollToTableRow(self, zone, tableLocator, tableIndex=0, rowIndex=0):
        """Scroll table / list to the given row
        Args:
                zone (str): Select Zone
                tableLocator (str): Select Table Locator
                tableIndex (int, 0): Table Locator Index
                rowIndex (int, 0): Row Index"""
        self.execute("elementScrollToTableRow", zone, tableLocator, tableIndex, rowIndex)

    def elementSendText(self, zone, element, index, text):
        """Send text to an element
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                text (str): Text to Send"""
        self.execute("elementSendText", zone, element, index, text)

    def elementSetProperty(self, zone, element, index, property, value):
        """Set element property
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                property (str): Property
                value (str): The value to set"""
        return self.execute("elementSetProperty", zone, element, index, property, value).get("text")

    def elementSwipe(self, zone, element, index, direction, offset=0, time=2000):
        """Swipe the screen in a given direction
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset
                time (int, 2000): Swipe overall time"""
        self.execute("elementSwipe", zone, element, index, direction, offset, time)

    def elementSwipeWhileNotFound(self, componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex=0, delay=1000, rounds=5, click=True):
        """Swipe a component to search for an element or text.
        Args:
                componentZone (str): Zone of the container element;
                componentElement (str): The container element;
                direction (str): Direction to swipe;
                offset (int, 0): Swipe offset;
                swipeTime (int, 2000): Swipe operation time;
                elementfindzone (str): Select Zone of the sought element;
                elementtofind (str): Select element to find from the drop-down list OR (for OCR text identification) insert text into the empty box in the drop-down list;
                elementtofindindex (int, 0): The sought element's index (i.e., the number of times the element appears after the first appearance minus two). Index=0 refers to the first appearance of the element; Index=1 refers to the second appearance of the element, etc.
                delay (int, 1000): Time to wait before sending a command (in milliseconds);
                rounds (int, 5): Maximum swipe rounds;
                click (boolean, true): Click the found element if TRUE."""
        return self.execute("elementSwipeWhileNotFound", componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex, delay, rounds, click).get("found")

    def endTransaction(self, name):
        """End measuring transaction duration
        Args:
                name (str): Name of the transaction"""
        self.execute("endTransaction", name)

    def exit(self):
        """Exit SeeTest."""
        self.execute("exit")

    def extractLanguageFiles(self, application, directoryPath, allowOverwrite=True):
        """Extracts the language files of an application to the specified directory
        Args:
                application (str): Language files source application
                directoryPath (str): Directory's full path where to extract the language files
                allowOverwrite (boolean, true): Whether to allow overwriting existing langauge files in directory"""
        self.execute("extractLanguageFiles", application, directoryPath, allowOverwrite)

    def findElements(self, zone, parent, by, value):
        return self.execute("findElements", zone, parent, by, value).get("textArray")

    def flick(self, direction, offset=0):
        """Flick the screen in a given direction
        Args:
                direction (str): Direction to flick
                offset (int, 0): Flick offset"""
        self.execute("flick", direction, offset)

    def flickCoordinate(self, x, y, direction):
        """Flick from a given point in a given direction
        Args:
                x (int, 0): Horizontal coordinate
                y (int, 0): Vertical coordinate
                direction (str): Direction to flick"""
        self.execute("flickCoordinate", x, y, direction)

    def flickElement(self, zone, element, index, direction):
        """Flick the element in a given direction
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                direction (str): Direction to flick"""
        self.execute("flickElement", zone, element, index, direction)

    def forceTouch(self, zone, element, index=0, duration=100, force=100, dragDistanceX=0, dragDistanceY=0, dragDuration=1500):
        """Force touch on element and drag for distance.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                duration (int, 100): Duration
                force (int, 100): Force level in percent
                dragDistanceX (int, 0): Horizontal distance of drag from Element
                dragDistanceY (int, 0): Vertical distance of drag from Element
                dragDuration (int, 1500): Drag Duration"""
        self.execute("forceTouch", zone, element, index, duration, force, dragDistanceX, dragDistanceY, dragDuration)

    def generateReport(self):
        """Generates a SeeTest report.
        Returns:
                str: returns the report's folder path"""
        return self.execute("generateReport").get("text")

    def generateReport2(self, releaseClient=True):
        """Generates a SeeTest report.
        Args:
                releaseClient (boolean, true): If FALSE then device will not be released
        Returns:
                str: returns the report's folder path"""
        return self.execute("generateReport", releaseClient).get("text")

    def generateReport(self, releaseClient, propFilePath):
        """Generates a SeeTest report and attach it to external tool entity.
			Suply a path to java properties file containning the data for the entity update.
			See documentation online for details.
        Args:
                releaseClient (boolean, true): If FALSE then device will not be released
                propFilePath (str): path to Java properties file
        Returns:
                str: returns the report's folder path, or the created entity if exists"""
        return self.execute("generateReport", releaseClient, propFilePath).get("text")

    def getAllSms(self, timeout=5000):
        """Get all  SMS or wait up to 'timeout' milliseconds.
        Args:
                timeout (int, 5000): Wait timeout"""
        return self.execute("getAllSms", timeout).get("textArray")

    def getAllValues(self, zone, element, property):
        """Get all the values of a property in a given element. Note: Supported properties are available on Object spy.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                property (str): Property"""
        return self.execute("getAllValues", zone, element, property).get("textArray")

    def getAllZonesWithElement(self, element):
        """Get all the zones names that has an element with the given name.
        Args:
                element (str): the element name to search for
        Returns:
                str: comma delimited string with the zones names"""
        return self.execute("getAllZonesWithElement", element).get("text")

    def getAvailableAgentPort(self):
        """Get available Agent Port.
			Get a port number where a SeeTest agent listens."""
        return self.execute("getAvailableAgentPort").get("port")

    def getAvailableAgentPort2(self, featureName):
        """Get available Agent Port.
			Get a port number of a SeeTest agent supporting (licensed to) the given
			device type.
        Args:
                featureName (str): A device type that is supported by the requested agent. One of ANDROID, IPHONE, BLACKBERRY or WINDOWS_PHONE."""
        return self.execute("getAvailableAgentPort", featureName).get("port")

    def getBrowserTabIdList(self):
        """Get an array of IDs of tabs in the opened browser (Chrome / Safari)."""
        return self.execute("getBrowserTabIdList").get("textArray")

    def getConnectedDevices(self):
        """Get connected devices."""
        return self.execute("getConnectedDevices").get("text")

    def getContextList(self):
        """Get the current context handlers list"""
        return self.execute("getContextList").get("textArray")

    def getCoordinateColor(self, x=0, y=0):
        """Returns an integer representation in the RGB color model for coordinate (x,y)
        Args:
                x (int, 0): the X coordinate of the target pixel
                y (int, 0): the Y coordinate of the target pixel"""
        return self.execute("getCoordinateColor", x, y).get("color")

    def getCounter(self, counterName):
        """Get monitor counter value
        Args:
                counterName (str): Counter name (cpu, memory...)"""
        return self.execute("getCounter", counterName).get("text")

    def getCurrentApplicationName(self):
        """Get the name of application that is running in the foreground of the device"""
        return self.execute("getCurrentApplicationName").get("text")

    def getCurrentBrowserTabId(self):
        """Get the ID of the currently focused browser (Chrome / Safari) tab.
        Returns:
                str: ID of the current tab"""
        return self.execute("getCurrentBrowserTabId").get("text")

    def getDefaultTimeout(self):
        """Gets the default timeout.
        Returns:
                int: the default timeout"""
        return self.execute("getDefaultTimeout").get("timeout")

    def getDeviceLog(self):
        """Download device log to reports directory
        Returns:
                str: path to the file that was downloaded"""
        return self.execute("getDeviceLog").get("path")

    def getDeviceProperty(self, key):
        """Get device property value for the given key.
        Args:
                key (str): device_property
        Returns:
                str: The property value"""
        return self.execute("getDeviceProperty", key).get("text")

    def getDevicesInformation(self):
        """Get an XML formated string containing all the devices information"""
        return self.execute("getDevicesInformation").get("text")

    def getElementCount(self, zone, element):
        """Count the number of times an element is been found in the current screen.
        Args:
                zone (str): the element zone
                element (str): the element name
        Returns:
                int: the number of times the elment was identified"""
        return self.execute("getElementCount", zone, element).get("count")

    def getElementCountIn(self, zoneName, elementSearch, index, direction, elementCountZone, elementCount, width=0, height=0):
        """Search for an element and count the number of times an element is found near him.The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zoneName (str): Select Zone
                elementSearch (str): Element Search
                index (int, 0): Element index
                direction (str): Direction to analyze
                elementCountZone (str): Select Zone
                elementCount (str): Element to count
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        return self.execute("getElementCountIn", zoneName, elementSearch, index, direction, elementCountZone, elementCount, width, height).get("count")

    def getInstalledApplications(self):
        """Get a string containing all installed application on the device"""
        return self.execute("getInstalledApplications").get("text")

    def getLastSMS(self, timeout=5000):
        """Get last received SMS or wait up to 'timeout' milliseconds to receive one.
        Args:
                timeout (int, 5000): Wait timeout"""
        return self.execute("getLastSMS", timeout).get("text")

    def getMonitorsData(self, cSVfilepath):
        """Returns a CSV format of the running monitors (CPU/Memomy) for all
      devices
        Args:
                cSVfilepath (str): If set will save the CSV in the given location (should be absolute path), if not set will use a default location."""
        return self.execute("getMonitorsData", cSVfilepath).get("text")

    def getNetworkConnection(self, connection):
        """getting the network connection for a device.
			Execute any of the following connection type:
			airplane_mode, wifi, mobile_data.
        Args:
                connection (str): Name of connection to get status"""
        return self.execute("getNetworkConnection", connection).get("found")

    def getNVProfiles(self):
        """Get all Network Virtualization profiles."""
        return self.execute("getNVProfiles").get("textArray")

    def getPickerValues(self, zone, pickerElement, index=0, wheelIndex=0):
        """Get all values from picker, works only on iOS
        Args:
                zone (str): Select Zone
                pickerElement (str): Select Picker Element
                index (int, 0): Picker index
                wheelIndex (int, 0): Wheel index at picker component"""
        return self.execute("getPickerValues", zone, pickerElement, index, wheelIndex).get("textArray")

    def getPosition(self, zone, element):
        """Gets position an position. The position is absolute.
        Args:
                zone (str): the element zone
                element (str): Select Element to Get Position For
        Returns:
                str: the comma delimited string with the X,Y of the element coordination."""
        return self.execute("getPosition", zone, element).get("click")

    def getPositionWindowRelative(self, zone, element):
        """Get the element position relative to the window (and not the screen absolute coordinates)
        Args:
                zone (str): Select Zone
                element (str): Select Element to Get Position For
        Returns:
                str: the comma delimited string with the X,Y of the element coordination."""
        return self.execute("getPositionWindowRelative", zone, element).get("centerWinRelative")

    def getProperty(self, property):
        """Get SeeTest's property value for the given key (in %appdata%\seetest\app.properties file).
        Args:
                property (str): The property key.
        Returns:
                str: the property value"""
        return self.execute("getProperty", property).get("text")

    def getSimCard(self):
        """Get sim-card name assigned to the currently used device"""
        return self.execute("getSimCard").get("text")

    def getSimCards(self, readyToUse=True):
        """Get list of sim-card names which are either ready to use or already assigned (depeand on the argument) to the currently used device
        Args:
                readyToUse (boolean, true): If TRUE return all available sim cards which are ready to use. If FALSE return all sim cards assigned for the device"""
        return self.execute("getSimCards", readyToUse).get("textArray")

    def getTableCellText(self, zone, headerElement, headerIndex, rowElement, rowIndex=0, width=0, height=0):
        """Get text content of table cell by its header element and row element.
        Args:
                zone (str): Select zone
                headerElement (str): Select table header element
                headerIndex (int, 0): Header element index
                rowElement (str): Select table row element
                rowIndex (int, 0): Row element index
                width (int, 0): Width of the search
                height (int, 0): Height of the search"""
        return self.execute("getTableCellText", zone, headerElement, headerIndex, rowElement, rowIndex, width, height).get("text")

    def getText(self, zone):
        """Gets the text that appears in a specified zone.
        Args:
                zone (str): Select the Zone to Get Text From, 'TEXT' and 'NATIVE' can be used as well.
        Returns:
                str: the text"""
        return self.execute("getText", zone).get("text")

    def getTextIn(self, zone, element, index, direction, width=0, height=0):
        """Get a text in a specified area indicate by an element, direction, width and height.
			The direction can be UP, DOWN, LEFT and RIGHT.
			Use this method to get the text found in an area related to a given element.
			When using zone NATIVE, only leafs elements will be searched in order to prevent multiple
			occurrences of the same text (containers can include the sub elements text too).
        Args:
                zone (str): Anchor Zone
                element (str): Anchor Element
                index (int, 0): Anchor Element index
                direction (str): Direction to analyze
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        return self.execute("getTextIn", zone, element, index, direction, width, height).get("text")

    def getTextIn2(self, zone, element, index, textZone, direction, width=0, height=0):
        """Get a text in a specified area indicate by an element,	direction, width and height. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                textZone (str): The zone to extract the text from
                direction (str): Direction to analyze
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        return self.execute("getTextIn", zone, element, index, textZone, direction, width, height).get("text")

    def getTextIn3(self, zone, element, index, textZone, direction, width=0, height=0, xOffset=0, yOffset=0):
        """Get the text in a specific area relative to an element, index, direction, width and height. Direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                textZone (str): The zone to extract the text from
                direction (str): Direction to analyze
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)
                xOffset (int, 0): identification rectangle x offset
                yOffset (int, 0): identification rectangle y offset"""
        return self.execute("getTextIn", zone, element, index, textZone, direction, width, height, xOffset, yOffset).get("text")

    def getVisualDump(self, type="Native"):
        """Get visual dump
        Args:
                type (str, 'Native'): Set the dump type"""
        return self.execute("getVisualDump", type).get("text")

    def hybridClearCache(self, clearCookies=True, clearCache=True):
        """Clear browser cookies and/or cache
        Args:
                clearCookies (boolean, true): If true cookies will be cleared
                clearCache (boolean, true): If true cache will be cleared"""
        resultMap = self.execute("hybridClearCache", clearCookies, clearCache)
        if (self.isThrowExceptionOnFail()):
            if (resultMap.get("errorMassage")):
                assert resultMap.get("errorMassage")

    def hybridGetHtml(self, webViewLocator, index=0):
        """Get HTML content from a webView element
        Args:
                webViewLocator (str): WebView locator string like id=web or empty for the first WebView in page
                index (int, 0): Element index"""
        return self.execute("hybridGetHtml", webViewLocator, index).get("text")

    def hybridRunJavascript(self, webViewLocator, index, script):
        """Run Javascript in a WebView.
        Args:
                webViewLocator (str): WebView locator string like id=web or empty for the first WebView in page
                index (int, 0): Element index
                script (str): Javascript"""
        return self.execute("hybridRunJavascript", webViewLocator, index, script).get("text")

    def hybridSelect(self, webViewLocator, index, method, value, select):
        """Select an option from a select element in a WebView. Using the input method and value to identify the element
        Args:
                webViewLocator (str): WebView locator string like id=web or empty for the first WebView in page
                index (int, 0): Element index
                method (str): Identification method (css/id)
                value (str): Identification value
                select (str): Option to select"""
        self.execute("hybridSelect", webViewLocator, index, method, value, select)

    def hybridWaitForPageLoad(self, timeout=10000):
        """Wait for web page to load
        Args:
                timeout (int, 10000): Waiting Timeout in MiliSec"""
        self.execute("hybridWaitForPageLoad", timeout)

    def install(self, path, sign=False):
        """Install the application in the given path on the device
        Args:
                path (str): can be an APK/IPA etc. absolute path or an activity name from the application manager.
                sign (boolean, false): If set to TRUE will sign the application (if not already signed)
        Returns:
                boolean: installation success"""
        return self.execute("install", path, sign).get("found")

    def install2(self, path, instrument=True, keepData=False):
        """Install the application in the given path on the device
        Args:
                path (str): can be an APK/IPA etc. absolute path or an activity name from the application manager.
                instrument (boolean, true): If set to TRUE will sign the application (if not already instrumented)
                keepData (boolean, false): If set to TRUE will keep application data
        Returns:
                boolean: installation success"""
        return self.execute("install", path, instrument, keepData).get("found")

    def installWithCustomKeystore(self, path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword):
        """Install instrumented application Signed with a given keystore
        Args:
                path (str): can be an APK/IPA etc. absolute path or an activity name from the application manager.
                instrument (boolean, true): If set to TRUE will sign the application (if not already instrumented)
                keepData (boolean, false): If set to TRUE will keep application data
                keystorePath (str): Key-store file location on Disk
                keystorePassword (str): The key-store password
                keyAlias (str): The key alias
                keyPassword (str): The key password
        Returns:
                boolean: installation success"""
        return self.execute("installWithCustomKeystore", path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword).get("found")

    def isElementBlank(self, zone, element, index=0, colorGroups=10):
        """Check if a given element found in the specified zone is blank; if blank returns TRUE if not found returns FALSE
        Args:
                zone (str): Select Zone
                element (str): Element to find
                index (int, 0): Element index
                colorGroups (int, 10): The number of color groups that indicate an image"""
        return self.execute("isElementBlank", zone, element, index, colorGroups).get("found")

    def isElementFound2(self, zone, element):
        """Check if a given element is found in the specified zone; if the element found returns TRUE if not found returns FALSE
        Args:
                zone (str): Select Zone
                element (str): Element to find"""
        return self.execute("isElementFound", zone, element).get("found")

    def isElementFound(self, zone, element, index=0):
        """Check if a given element is found in the specified zone; if found returns TRUE if not found returns FALSE
        Args:
                zone (str): Select Zone
                element (str): Element to find
                index (int, 0): Element index"""
        return self.execute("isElementFound", zone, element, index).get("found")

    def isFoundIn(self, zone, searchElement, index, direction, elementFindZone, elementToFind, width=0, height=0):
        """Search for an element and check if an element related to it exist. The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                elementFindZone (str): Find Element Zone
                elementToFind (str): Element to Find
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        return self.execute("isFoundIn", zone, searchElement, index, direction, elementFindZone, elementToFind, width, height).get("found")

    def isTextFound(self, zone, element, ignoreCase):
        """Check if a given text is found in the specified zone; if the element found returns TRUE if not found returns FALSE
        Args:
                zone (str): Select Zone
                element (str): Element to find
                ignoreCase (boolean): should command case sensitivity"""
        return self.execute("isTextFound", zone, element, ignoreCase).get("found")

    def launch2(self, applicationIdentifier, launchOptions):
        """Launch application by identifier (bundleID / package and activity) and an options map.
			An empty map denotes default launch behaviour.

			Some of the available options are platform dependent, and some are generic. Currently supported options:

			Generic Options:
			- "relaunch"     (boolean): whether an existing process of this application should be killed before launching the application.
			- "instrumented" (boolean): whether the application should be launched in instrumented mode or not.

			iOS Specific Options:
			- "launch_args"  (list/array)     : this list (of strings) will be appended to the launched application command line.
			- "launch_env"   (map/dictionary) : a (string to string) key value pairs of environment variables to set for the application.

			Android Specific Options:
			(No Android Specific Options are implemented yet).
        Args:
                applicationIdentifier (str): The application's bundle ID / package name
                launchOptions (str): A map / dictionary containing launch options"""
        self.execute("launch", applicationIdentifier, launchOptions)

    def launch(self, activityURL, instrument=True, stopIfRunning=False):
        """Launch activity
        Args:
                activityURL (str): The application main activity or URL
                instrument (boolean, true): If set to true then will launch in instrument mode
                stopIfRunning (boolean, false): If set to true then will stop the running process before launching it"""
        self.execute("launch", activityURL, instrument, stopIfRunning)

    def listSelect(self, sendRest, sendNavigation, delay, textToIdentify, color, rounds=5, sendonfind="{ENTER}"):
        """Select an element from a list. Used for non-touch devices like blackberry.
        Args:
                sendRest (str, '{UP}'): Navigate to the list start
                sendNavigation (str, '{DOWN}'): Send to navigate in the list
                delay (int, 500): Time to wait before sending a command (in MiliSec)
                textToIdentify (str): Select text to Find
                color (str): Color to filter
                rounds (int, 5): Maximum navigation rounds
                sendonfind (str, '{ENTER}'): Send on text find
        Returns:
                boolean: true if element was found"""
        return self.execute("listSelect", sendRest, sendNavigation, delay, textToIdentify, color, rounds, sendonfind).get("found")

    def longClick(self, zone, element, index=0, clickCount=1, x=0, y=0):
        """Long click on or near to an element (the proximity to the element is specified by a X-Y offset).
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                clickCount (int, 1): Number of Clicks
                x (int, 0): Horizontal Offset from Element
                y (int, 0): Vertical Offset from Element"""
        self.execute("longClick", zone, element, index, clickCount, x, y)

    def maximize(self):
        """Maximize the working window"""
        self.execute("maximize")

    def multiClick(self, zone, element, index, fingerIndex):
        """Click on an element with a finger.
Note: element will be identified right before performing the gesutre
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                fingerIndex (int): Finger index"""
        self.execute("multiClick", zone, element, index, fingerIndex)

    def multiClickCoordinate(self, x, y, fingerIndex):
        """Click with a finger on given coordinate
        Args:
                x (int): Horizontal coordinate
                y (int): Vertical coordinate
                fingerIndex (int): Finger index"""
        self.execute("multiClickCoordinate", x, y, fingerIndex)

    def multiClickOffset(self, zone, element, index, x, y, fingerIndex):
        """Click with a finger near an element, in given offset form the element.
Note: element will be identified right before performing the gesutre
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                x (int): Horizontal coordinate
                y (int): Vertical coordinate
                fingerIndex (int): Finger index"""
        self.execute("multiClickOffset", zone, element, index, x, y, fingerIndex)

    def multiSwipe(self, direction, offset, time, fingerIndex):
        """Swipe the screen in a given direction
        Args:
                direction (str): Swipe direction
                offset (int): Swipe offset
                time (int, 500): Swipe overall time
                fingerIndex (int): Finger index"""
        self.execute("multiSwipe", direction, offset, time, fingerIndex)

    def multiTouchDown(self, zone, element, index, fingerIndex):
        """Hold a finger Touched down on an element. Release with MultiTouchUp command.
Note: element will be identified right before performing the gesture
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                fingerIndex (int): Finger index"""
        self.execute("multiTouchDown", zone, element, index, fingerIndex)

    def multiTouchDownCoordinate(self, x, y, fingerIndex):
        """Hold a finger Touched down on a given coordinate
        Args:
                x (int): Horizontal coordinate
                y (int): Vertical coordinate
                fingerIndex (int): Finger index"""
        self.execute("multiTouchDownCoordinate", x, y, fingerIndex)

    def multiTouchMove(self, zone, element, index, fingerIndex):
        """Gesture-move a finger to the element, from the last coordinate touched down or moved to.
Note: element will be identified right before performing the gesutre
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element index
                fingerIndex (int): Finger index"""
        self.execute("multiTouchMove", zone, element, index, fingerIndex)

    def multiTouchMoveCoordinate(self, x, y, fingerIndex):
        """Move a finger to the given coordinate, from the last coordinate touched down or moved to.
        Args:
                x (int): Horizontal coordinate
                y (int): Vertical coordinate
                fingerIndex (int): Finger index"""
        self.execute("multiTouchMoveCoordinate", x, y, fingerIndex)

    def multiTouchUp(self, fingerIndex):
        """Touch up with a finger, from the last coordinate touched down or moved to
        Args:
                fingerIndex (int): Finger index"""
        self.execute("multiTouchUp", fingerIndex)

    def multiWait(self, time, fingerIndex):
        """Wait with a finger in the last touch point, for the given time
        Args:
                time (int, 500): Wait time
                fingerIndex (int): Finger index"""
        self.execute("multiWait", time, fingerIndex)

    def openDevice(self):
        """Opens current device's screen."""
        self.execute("openDevice")

    def p2cx(self, percentage=0):
        """Convert percentage into pixel.
        Args:
                percentage (int, 0): Screen Percentage"""
        return self.execute("p2cx", percentage).get("pixel")

    def p2cy(self, percentage=0):
        """Convert percentage into pixel.
        Args:
                percentage (int, 0): Screen Percentage"""
        return self.execute("p2cy", percentage).get("pixel")

    def performMultiGesture(self):
        """Perform the multi-touch gesture steps which were added after the last StartMultiGestureStep command"""
        self.execute("performMultiGesture")

    def pinch(self, inside=True, x=0, y=0, radius=100):
        """Pinch In/Out at X,Y in specific radius, if X and Y equal to 0, the pinch will be performed from the center of the screen. In most cases, pinching IN would be used to ZOOM OUT, and pinching OUT would be used to ZOOM IN.
        Args:
                inside (boolean, true): Set 'true' to pinch in (zoom out), 'false' to pinch out (zoom in) 
                x (int, 0): X center
                y (int, 0): Y center
                radius (int, 100): The pinch radius"""
        return self.execute("pinch", inside, x, y, radius).get("found")

    def pinch2(self, inside=True, x=0, y=0, radius=100, horizontal=False):
        """Pinch In/Out at X,Y in specific radius, if X and Y equal to 0, the pinch will be performed from the center of the screen. In most cases, pinching IN would be used to ZOOM OUT, and pinching OUT would be used to ZOOM IN.
        Args:
                inside (boolean, true): Set 'true' to pinch in (zoom out), 'false' to pinch out (zoom in) 
                x (int, 0): X center
                y (int, 0): Y center
                radius (int, 100): The pinch radius
                horizontal (boolean, false): Vertical / horizontal pinch"""
        return self.execute("pinch", inside, x, y, radius, horizontal).get("found")

    def portForward(self, localPort, remotePort):
        """Forward an agent port
        Args:
                localPort (int): The local port
                remotePort (int): The remote port
        Returns:
                int: the remote port assigned"""
        return self.execute("portForward", localPort, remotePort).get("port")

    def pressWhileNotFound(self, zone, elementtoclick, elementtofind, timeout=10000, delay=0):
        """Press on a certain element (ElementToClick) while another element (ElementToFind)is not found.
        Args:
                zone (str): Select Zone
                elementtoclick (str): Select Element To Click
                elementtofind (str): Select Element To Find
                timeout (int, 10000): Waiting Timeout in MiliSec
                delay (int, 0): Time to wait before clicking on the ElementToClick (in MiliSec)"""
        self.execute("pressWhileNotFound", zone, elementtoclick, elementtofind, timeout, delay)

    def pressWhileNotFound2(self, zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex=0, timeout=10000, delay=0):
        """Press on a certain element (ElementToClick) while another element (ElementToFind)is not found.
        Args:
                zone (str): Select Zone
                elementtoclick (str): Select Element To Click
                elementtoclickindex (int, 0): Element To Click Index
                elementtofind (str): Select Element To Find
                elementtofindindex (int, 0): Element To Find Index
                timeout (int, 10000): Waiting Timeout in MiliSec
                delay (int, 0): Time to wait before clicking on the ElementToClick (in MiliSec)"""
        self.execute("pressWhileNotFound", zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex, timeout, delay)

    def reboot(self, timeout=120000):
        """Will reboot the device
        Args:
                timeout (int, 120000): Timeout waiting for the device to reboot. Minimum value is 40000.
        Returns:
                boolean: have device already been rebooted within given timeout"""
        return self.execute("reboot", timeout).get("status")

    def receiveIncomingCall(self, fromNumber, hangupInSeconds):
        """Initiate a 3rd party phone call to be received by the device.
        Args:
                fromNumber (str): Number that the call will come from
                hangupInSeconds (int): How long to wait before hanging up while ringing"""
        self.execute("receiveIncomingCall", fromNumber, hangupInSeconds)

    def receiveIncomingSMS(self, fromNumber, msg):
        """Initiate a 3rd party SMS to be received by the device.
        Args:
                fromNumber (str): Number that the SMS will come from
                msg (str): Text of the message to receive"""
        self.execute("receiveIncomingSMS", fromNumber, msg)

    def releaseClient(self):
        """Release Controller."""
        self.execute("releaseClient")

    def releaseDevice(self, deviceName, releaseAgent=True, removeFromDeviceList=False, releaseFromCloud=True):
        """Release device as well as the agent assigned to it. It will enable other tests that are waiting to be executed to start doing so.
        Args:
                deviceName (str): Name of the device to release
                releaseAgent (boolean, true): Deprecated. If TRUE or FALSE then agent will be released
                removeFromDeviceList (boolean, false): If TRUE then the device will be remove from the device list
                releaseFromCloud (boolean, true): If TRUE then the device will be released from the cloud"""
        self.execute("releaseDevice", deviceName, releaseAgent, removeFromDeviceList, releaseFromCloud)

    def report(self, message, status):
        """Add a step with a custom message to the test report.
        Args:
                message (str): The message to report.
                status (boolean): TRUE marks this step as successfully passed, FALSE marks it as failed."""
        self.execute("report", message, status)

    def reportWithImage(self, pathToImage, message, status):
        """Add a step with a custom message and image to the test report.
        Args:
                pathToImage (str): Pass to the image, can be either a local full path or a URL.
                message (str): The message to report.
                status (boolean): TRUE marks this step as successfully passed, FALSE marks it as failed."""
        self.execute("report", pathToImage, message, status)

    def resetDeviceBridge(self):
        """Reset Device Bridge."""
        self.execute("resetDeviceBridge")

    def resetDeviceBridgeOS(self, deviceType):
        """Reset Device Bridge.
        Args:
                deviceType (str): A device type that is supported by the requested agent"""
        self.execute("resetDeviceBridge", deviceType)

    def rightClick(self, zone, element, index=0, clickCount=1, x=0, y=0):
        """Right-click on or near to an element (the proximity to the element is specified by a X-Y offset)
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order
                clickCount (int, 1): Number of Clicks
                x (int, 0): Horizontal Offset from Element
                y (int, 0): Vertical Offset from Element"""
        self.execute("rightClick", zone, element, index, clickCount, x, y)

    def run(self, command):
        """Run the given command line as shell command on the device (adb for android and SSH for iOS).
			When running an executable a full path	should be given.
        Args:
                command (str): The command to run"""
        return self.execute("run", command).get("text")

    def LayoutTest(self, xml):
        """Run a layout test
        Args:
                xml (str): the xml of the layout tests"""
        return self.execute("runLayoutTest", xml).get("text")

    def runNativeAPICall(self, zone, element, index, script):
        """Run native API call on the given element.
        Args:
                zone (str): Select Zone (should be NATIVE)
                element (str): Select Element (should use xpath)
                index (int, 0): Element index
                script (str): Script to execute"""
        return self.execute("runNativeAPICall", zone, element, index, script).get("text")

    def sendText(self, text):
        """Sends/inserts text to the application. Supports the following keyboard actions as well: {ENTER}, {BKSP}. Device actions can now be found under DeviceAction command.
        Args:
                text (str): Text to send"""
        self.execute("sendText", text)

    def sendWhileNotFound(self, toSend, zone, elementtofind, timeout=10000, delay=1000):
        """Send a given text while an element is not found
        Args:
                toSend (str): Insert Text To Send
                zone (str): Select Zone
                elementtofind (str): Select Element to Find
                timeout (int, 10000): Waiting Timeout in MiliSec
                delay (int, 1000): Time to wait before sending a command (in MiliSec)"""
        self.execute("sendWhileNotFound", toSend, zone, elementtofind, timeout, delay)

    def sendWhileNotFound2(self, toSend, zone, elementtofind, elementtofindindex=0, timeout=10000, delay=1000):
        """Send a given text while an element is not found
        Args:
                toSend (str): Insert Text To Send
                zone (str): Select Zone
                elementtofind (str): Select Element to Find
                elementtofindindex (int, 0): Element to Find Index
                timeout (int, 10000): Waiting Timeout in MiliSec
                delay (int, 1000): Time to wait before sending a command (in MiliSec)"""
        self.execute("sendWhileNotFound", toSend, zone, elementtofind, elementtofindindex, timeout, delay)

    def setApplicationTitle(self, title):
        """Sets the application title.
	 		Switch tested application/window.
	 		The title of the application is searched in the following order:
	 		1. search for windows with the given window ID (only if the title is a number).
	 		2. search for complete match.
	 		3. search for partial match (contains).
	 		4. Regular expression match.
	 		When working on the desktop use the 'desktop' as the application title.
        Args:
                title (str): Insert New Application/Window Title"""
        self.execute("setApplicationTitle", title)

    def setAuthenticationReply(self, reply="Success", delay=0):
        """Set the reply type for later authentication requests.
See online documentation for details.
        Args:
                reply (str, 'Success'): The reply type to mock
                delay (int, 0): Delay after request in ms"""
        self.execute("setAuthenticationReply", reply, delay)

    def setContext(self, context="NATIVE_APP"):
        """Set the current context
        Args:
                context (str, 'NATIVE_APP'): Context"""
        self.execute("setContext", context)

    def setDefaultClickDownTime(self, downTime=100):
        """Set the default click down time in milliseconds (default is 100)
        Args:
                downTime (int, 100): Click down time value"""
        self.execute("setDefaultClickDownTime", downTime)

    def setDefaultTimeout(self, newTimeout=20000):
        """Set the default timeout for click commands
        Args:
                newTimeout (int, 20000): Waiting Timeout in MiliSec
        Returns:
                str: the new timeout"""
        return self.execute("setDefaultTimeout", newTimeout).get("timeout")

    def setDefaultWebView(self, webViewLocator):
        """Set the WebView to work with
        Args:
                webViewLocator (str): The locator of the WebView, empty to reset"""
        self.execute("setDefaultWebView", webViewLocator)

    def setDevice(self, device):
        """Sets the device.
        Args:
                device (str): Device name"""
        self.execute("setDevice", device)

    def setDragStartDelay(self, delay=0):
        """Drag called after setting dragStartDelay will begin by long touch on the device in
			the initial drag location.
        Args:
                delay (int, 0): Delay value"""
        self.execute("setDragStartDelay", delay)

    def setInKeyDelay(self, delay=50):
        """Set the time between key down and key up
        Args:
                delay (int, 50): Delay value"""
        self.execute("setInKeyDelay", delay)

    def setKeyToKeyDelay(self, delay=50):
        """Set the time between two key press
        Args:
                delay (int, 50): Delay value"""
        self.execute("setKeyToKeyDelay", delay)

    def setLanguage(self, language):
        """Set the OCR language
        Args:
                language (str): The language to use or empty to reset to English"""
        self.execute("setLanguage", language)

    def setLanguagePropertiesFile(self, propertiesfile):
        """Set language properties file
        Args:
                propertiesfile (str): The language properties file path"""
        self.execute("setLanguagePropertiesFile", propertiesfile)

    def setLocation(self, latitude="0.0", longitude="0.0"):
        """Sets device's location to specified coordinate. Currently supported only on android
        Args:
                latitude (str, '0.0'): Latitude in decimal degrees from -90 to 90. Positive latitudes are north of the equator and negative latitudes are south of the equator.
                longitude (str, '0.0'): Longitude in decimal degrees from -180 to 180. Positive longitudes are east of the Prime Meridian and negative longitudes are west of the Prime Meridian"""
        self.execute("setLocation", latitude, longitude)

    def setMonitorPollingInterval(self, timemilli=30000):
        """Set monitor polling interval
        Args:
                timemilli (int, 30000): The monitor polling time interval in milliseconds"""
        self.execute("setMonitorPollingInterval", timemilli)

    def setMonitorTestState(self, testStatus):
        """Set the test status to the monitors
        Args:
                testStatus (str): Set the current status of the test to be used by the monitors"""
        self.execute("setMonitorTestState", testStatus)

    def setNetworkConditions(self, profile):
        """Set profile for current device. To cancel profile leave the profile name empty
        Args:
                profile (str): Set the profile to be used"""
        self.execute("setNetworkConditions", profile)

    def setNetworkConditions2(self, profile, duration=0):
        """Set profile for current device for a specified duration. To cancel profile leave the profile name empty
        Args:
                profile (str): Set the profile to be used
                duration (int, 0): Set the duration in milliseconds to which to set the profile (0=forever)"""
        self.execute("setNetworkConditions", profile, duration)

    def setNetworkConnection(self, connection, enable):
        """setting the network connection for a device.
			Execute any of the following connection type:
			airplane_mode, wifi, mobile_data.
        Args:
                connection (str): Name of connection to set
                enable (boolean): Status of connection to set"""
        self.execute("setNetworkConnection", connection, enable)

    def setOcrIgnoreCase(self, ignoreCase):
        """Set the ignore case status (default is true).
        Args:
                ignoreCase (boolean): should OCR avoid case sensitivity"""
        self.execute("setOcrIgnoreCase", ignoreCase)

    def setOcrTrainingFilePath(self, trainingPath):
        self.execute("setOcrTrainingFilePath", trainingPath)

    def setPickerValues(self, zoneName, elementName, index, wheelIndex, value):
        """Set the values of the Picker element
        Args:
                zoneName (str): Select Zone
                elementName (str): Select Element
                index (int, 0): Element index
                wheelIndex (int): Wheel Index
                value (str): The value to set"""
        return self.execute("setPickerValues", zoneName, elementName, index, wheelIndex, value).get("text")

    def setProjectBaseDirectory(self, projectBaseDirectory):
        """Sets the project base directory.
			This method should be called first,	before any other called is used.
			In case of working on a remote agent, the project path should be the path on the remote machine.
        Args:
                projectBaseDirectory (str): the new project base directory."""
        self.execute("setProjectBaseDirectory", projectBaseDirectory)

    def setProperty(self, key, value):
        """
        Args:
                key (str): key
                value (str): Value"""
        self.execute("setProperty", key, value)

    def setRedToBlue(self, redToBlue):
        self.execute("setRedToBlue", redToBlue)

    def setReporter(self, reporterName, directory):
        """Sets the reporter. Configure the internal reporter.
        Args:
                reporterName (str, 'html'): Comma seperated value string of reporter types. Supported types: html(=xml), pdf.
                directory (str): The directory for the report to be generated in.
        Returns:
                str: The reports directory path"""
        return self.execute("setReporter", reporterName, directory).get("text")

    def setReporter2(self, reporterName, directory, testName):
        """Sets the reporter. Configure the internal reporter.
        Args:
                reporterName (str, 'html'): Comma seperated value string of reporter types. Supported types: html(=xml), pdf.
                directory (str): The directory for the report to be generated in.
                testName (str): The name of the test as would appear in the report.
        Returns:
                str: The reports directory path"""
        return self.execute("setReporter", reporterName, directory, testName).get("text")

    def setShowImageAsLink(self, showImageAsLink):
        """Show report images as link.
        Args:
                showImageAsLink (boolean): show report images as link"""
        self.execute("setShowImageAsLink", showImageAsLink)

    def setShowImageInReport(self, showImageInReport=True):
        """Decide whether to include step screenshots in report.
        Args:
                showImageInReport (boolean, true): when set to False will not show any screenshot in report"""
        self.execute("setShowImageInReport", showImageInReport)

    def setShowPassImageInReport(self, showPassImageInReport=True):
        """Decide whether to show screenshots of every step, or only failed steps in report.
        Args:
                showPassImageInReport (boolean, true): when set to False will not show a screenshot of test steps that passed successfully"""
        self.execute("setShowPassImageInReport", showPassImageInReport)

    def setShowReport(self, showReport=True):
        """when set to False will not show reports steps.
        Args:
                showReport (boolean, true): when set to False will not show reports steps"""
        self.execute("setShowReport", showReport)

    def setSimCard(self, simCardName):
        """Load or unload sim card (to unlaod sim-card name is equal to null) to the currently used device
        Args:
                simCardName (str): the sim-card name, null to unload sim card form the device"""
        self.execute("setSimCard", simCardName)

    def setSpeed(self, speed):
        self.execute("setSpeed", speed)

    def setTestStatus(self, status, message):
        """Set the test as passed or failed
        Args:
                status (boolean, true): Status
                message (str): Pass/Fail reason."""
        self.execute("setTestStatus", status, message)

    def setWebAutoScroll(self, autoScroll=True):
        """Set web autoscroll
        Args:
                autoScroll (boolean, true): Set the autoscroll state"""
        self.execute("setWebAutoScroll", autoScroll)

    def setWindowSize(self, width=0, height=0):
        """Set the active windows size and movie it to be fully visible.
        Args:
                width (int, 0): Window width
                height (int, 0): Window height"""
        self.execute("setWindowSize", width, height)

    def shake(self):
        """Simulating Shake operation on the device."""
        self.execute("shake")

    def simulateCapture(self, picturePath):
        """SimulateCapture
        Args:
                picturePath (str): picture's full path"""
        self.execute("simulateCapture", picturePath)

    def sleep(self, time=1000):
        """Pause the script for a specified time.
        Args:
                time (int, 1000): The time to pause in MiliSec"""
        self.execute("sleep", time)

    def startAudioPlay(self, audioFile):
        """Start play audio file
        Args:
                audioFile (str): Set the audio file to start playing"""
        self.execute("startAudioPlay", audioFile)

    def startAudioRecording(self, audioFile):
        """Start record audio file
        Args:
                audioFile (str): Set the audio file name"""
        self.execute("startAudioRecording", audioFile)

    def startCall(self, skypeUser, skypePassword, number, duration=0):
        """Generate a phone call using Skype infrastructure
        Args:
                skypeUser (str): Skype user
                skypePassword (str): Skype password
                number (str): Number to call include country code
                duration (int, 0): Call duration (0 is infinite or until EndCall)"""
        self.execute("startCall", skypeUser, skypePassword, number, duration)

    def startLoggingDevice(self, path):
        """Start writing the current device's log to file
        Args:
                path (str): Absolute path to log file, or where the log file should be created"""
        self.execute("startLoggingDevice", path)

    def startMonitor(self, packageName):
        """Clear monitoring collection data collected so far. If packageName is not empty, the application identified by this packageName will start being monitored.
        Args:
                packageName (str): Package name (Android) or Bundle ID (iOS) of application"""
        self.execute("startMonitor", packageName)

    def startMultiGesture(self, name):
        """Start a group of multi-touch gesture steps.
Note: all steps will be performed only when executing the PerformMultiGestureStep command.
See online documentation for details
        Args:
                name (str): Gesture name"""
        self.execute("startMultiGesture", name)

    def startStepsGroup(self, caption):
        """Start grouping steps
        Args:
                caption (str): The group's caption which will appear at the report"""
        self.execute("startStepsGroup", caption)

    def startTransaction(self, name):
        """Start to measure transaction duration
        Args:
                name (str): Name of the transaction"""
        self.execute("startTransaction", name)

    def startVideoRecord(self):
        """Start the video recording."""
        self.execute("startVideoRecord")

    def stopAudioPlay(self):
        """Stop audio playing"""
        self.execute("stopAudioPlay")

    def stopAudioRecording(self):
        """Stop audio recording"""
        self.execute("stopAudioRecording")

    def stopLoggingDevice(self):
        """Stop writing the current device's log to file"""
        return self.execute("stopLoggingDevice").get("text")

    def stopStepsGroup(self):
        """Stop grouping steps"""
        self.execute("stopStepsGroup")

    def stopVideoRecord(self):
        """Stop the video recording."""
        return self.execute("stopVideoRecord").get("text")

    def swipe(self, direction, offset=0):
        """Swipe the screen in a given direction
        Args:
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset"""
        self.execute("swipe", direction, offset)

    def swipe2(self, direction, offset=0, time=500):
        """Swipe the screen in a given direction
        Args:
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset
                time (int, 500): Swipe overall time"""
        self.execute("swipe", direction, offset, time)

    def swipeWhileNotFound(self, direction, offset, zone, elementtofind, delay=1000, rounds=5, click=True):
        """Swipe a list to identify an element
        Args:
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset
                zone (str): Select Zone
                elementtofind (str): Select Element to Find
                delay (int, 1000): Time to wait before sending a command (in MiliSec)
                rounds (int, 5): Maximum swipe rounds
                click (boolean, true): If TRUE then click"""
        return self.execute("swipeWhileNotFound", direction, offset, zone, elementtofind, delay, rounds, click).get("found")

    def swipeWhileNotFound3(self, direction, offset, swipeTime, zone, elementtofind, delay=1000, rounds=5, click=True):
        """Swipe a list to identify an element
        Args:
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset
                swipeTime (int, 2000): Swipe operation time
                zone (str): Select Zone
                elementtofind (str): Select Element to Find
                delay (int, 1000): Time to wait before sending a command (in MiliSec)
                rounds (int, 5): Maximum swipe rounds
                click (boolean, true): If TRUE then click"""
        return self.execute("swipeWhileNotFound", direction, offset, swipeTime, zone, elementtofind, delay, rounds, click).get("found")

    def swipeWhileNotFound2(self, direction, offset, swipeTime, zone, elementtofind, elementtofindindex=0, delay=1000, rounds=5, click=True):
        """Swipe a list to identify an element
        Args:
                direction (str): Direction to swipe
                offset (int, 0): Swipe offset
                swipeTime (int, 2000): Swipe operation time
                zone (str): Select Zone
                elementtofind (str): Select Element to Find
                elementtofindindex (int, 0): Element to Find Index
                delay (int, 1000): Time to wait before sending a command (in MiliSec)
                rounds (int, 5): Maximum swipe rounds
                click (boolean, true): If TRUE then click"""
        return self.execute("swipeWhileNotFound", direction, offset, swipeTime, zone, elementtofind, elementtofindindex, delay, rounds, click).get("found")

    def switchToBrowserTab(self, tabId):
        """Get the ID of the currently focused browser (Chrome / Safari) tab.
        Args:
                tabId (str): Tab ID to switch to"""
        self.execute("switchToBrowserTab", tabId)

    def sync(self, silentTime=2000, sensitivity=0, timeout=10000):
        """Wait for the screen to be silent. Works on the graphical level of the screen.
        Args:
                silentTime (int, 2000): The time for the screen to be silent in milliseconds
                sensitivity (int, 0): Sensitivity from 0 - 100 (0 is maximum sensitivity)
                timeout (int, 10000): Waiting timeout in milliseconds
        Returns:
                boolean: true if operation finished successfully"""
        return self.execute("sync", silentTime, sensitivity, timeout).get("found")

    def syncElements(self, silentTime=2000, timeout=10000):
        """Wait for all the UI elements on the page to appear. Works on the dump level - checks for changes in the UI dump.
        Args:
                silentTime (int, 2000): The period of time for the UI elements to be static in milliseconds.
                timeout (int, 10000): Waiting timeout in milliseconds
        Returns:
                boolean: true if operation finished successfully"""
        return self.execute("syncElements", silentTime, timeout).get("found")

    def textFilter(self, color, sensitivity=15):
        """Filter the text recognition text color for the next command.
			This setting will be applied on the next command only.
			It's only supported when you use text recognition (OCR).
        Args:
                color (str): text color in RGB hex decimal string (0xFFFFFF for white).
                sensitivity (int, 15): Sensitivity from 0 - 100 (0 is no sensitivity)"""
        self.execute("textFilter", color, sensitivity)

    def touchDown(self, zone, element, index=0):
        """Hold Touch down on element, Release with TouchUp command.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order"""
        self.execute("touchDown", zone, element, index)

    def touchDownCoordinate(self, x=0, y=0):
        """Hold Touch down at X,Y coordinates related to the device screen, Release with TouchUp command.
        Args:
                x (int, 0): Horizontal coordinate
                y (int, 0): Vertical coordinate"""
        self.execute("touchDownCoordinate", x, y)

    def touchMove(self, zone, element, index=0):
        """Move from last Touched down element/coordinate to specified element location.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order"""
        self.execute("touchMove", zone, element, index)

    def touchMoveCoordinate(self, x=0, y=0):
        """Move from last Touched down element/coordinate to specified coordinate location.
        Args:
                x (int, 0): Horizontal coordinate
                y (int, 0): Vertical coordinate"""
        self.execute("touchMoveCoordinate", x, y)

    def touchUp(self):
        """Touch Up from last coordinate touched down or moved to."""
        self.execute("touchUp")

    def uninstall(self, application):
        """Uninstall the application
        Args:
                application (str): The application name
        Returns:
                boolean: uninstallation success"""
        return self.execute("uninstall", application).get("found")

    def verifyElementFound(self, zone, element, index=0):
        """Check if an element is found in a specified zone.
			An Exception (or Assertion) will be thrown if the element is not found.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order (=the number of times the element appears more and above the first time)"""
        resultMap = self.execute("verifyElementFound", zone, element, index)
        if (self.isThrowExceptionOnFail()):
            if (resultMap.get("errorMassage")):
                assert resultMap.get("errorMassage")

    def verifyElementNotFound(self, zone, element, index=0):
        """Check if an element is found in a specified zone.
			An Exception (or Assertion) will be thrown if the element is found.
        Args:
                zone (str): Select Zone
                element (str): Select Element
                index (int, 0): Element Order (=the number of times the element appears more and above the first time)"""
        resultMap = self.execute("verifyElementNotFound", zone, element, index)
        if (self.isThrowExceptionOnFail()):
            if (resultMap.get("errorMassage")):
                assert resultMap.get("errorMassage")

    def verifyIn(self, zone, searchElement, index, direction, elementFindZone, elementToFind, width=0, height=0):
        """Verify that an element("elementToFind") is found near to another element("elementSearch").
			The direction can be UP, DOWN, LEFT and RIGHT.
        Args:
                zone (str): Select Zone
                searchElement (str): Search Element
                index (int, 0): Element index
                direction (str): Direction to analyze
                elementFindZone (str): Find Element Zone
                elementToFind (str): Element to Find
                width (int, 0): Width of the search (0 indicate until the end/start of the window)
                height (int, 0): Height of the search (0 indicate until the end/start of the window)"""
        resultMap = self.execute("verifyIn", zone, searchElement, index, direction, elementFindZone, elementToFind, width, height)
        if (self.isThrowExceptionOnFail()):
            if (resultMap.get("errorMassage")):
                assert resultMap.get("errorMassage")

    def waitForAudioPlayEnd(self, timeout):
        """Wait for audio file play to end
        Args:
                timeout (int): Set the timeout"""
        self.execute("waitForAudioPlayEnd", timeout)

    def waitForElement(self, zone, element, index=0, timeout=10000):
        """Wait for an element to appear in a specified zone
        Args:
                zone (str): Select Zone
                element (str): Select Element to Wait For
                index (int, 0): Element Order
                timeout (int, 10000): Waiting Timeout in MiliSec"""
        return self.execute("waitForElement", zone, element, index, timeout).get("found")

    def waitForElementToVanish(self, zone, element, index=0, timeout=10000):
        """Wait for an element to vanish
        Args:
                zone (str): Select Zone
                element (str): Select Element to Wait For
                index (int, 0): Element Order
                timeout (int, 10000): Waiting Timeout in MiliSec"""
        return self.execute("waitForElementToVanish", zone, element, index, timeout).get("found")

    def waitForWindow(self, name, timeout=10000):
        """Wait for a window
        Args:
                name (str): Window name
                timeout (int, 10000): Waiting Timeout in MiliSec"""
        return self.execute("waitForWindow", name, timeout).get("found")


        


            
if __name__ == "__main__":
    pass
