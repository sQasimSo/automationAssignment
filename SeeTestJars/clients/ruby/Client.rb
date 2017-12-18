#!/usr/bin/ruby -w
require 'xmlrpc/client'
require 'securerandom'
require 'weakref'
require 'thread'

module Mobile
  class Listener
    def setClient(client)
      @client = client
    end

    def initialize()
    end

    def recover(type, xpath)
      raise NotImplementedError
    end
  end

  class KeepAlive
    def initialize(clientRef)
      @clientRef = clientRef
    end

    def run()
      until ((@stop) or (not @clientRef.weakref_alive?)) do
        begin
          #puts Time.now
          @clientRef.pingServer
          sleep(1)
        rescue Exception => e
          puts 'Error'
          puts e.message
        end
      end
    end

    def release()
      @stop = true
    end

    def isRunning()
      return !@stop
    end
  end

  class Client

    def initialize(host = '127.0.0.1', port = 8889, useSessionID = false, is_secured = false, sessionId = nil, url_path_prefix = '')
      self.init_client(host, port, useSessionID, is_secured, sessionId, url_path_prefix)
    end

    def init_client(host, port, useSessionID = true, is_secured, sessionId, url_path_prefix)
      @host = host
      @port = port

      scheme = is_secured ? 'https://' : 'http://'
      url_path_prefix = fixUrlPathPrefix(url_path_prefix)

      uri = URI(scheme + host + ':' + port.to_s + url_path_prefix +'/xmlrpc')

      @server = XMLRPC::Client.new_from_uri(uri)
      @server.timeout = 300000
      @clientMonitor = Mobile::KeepAlive.new(WeakRef.new(self))
      @clientID = nil
      @listeners = Hash.new
      @throwExceptionOnFailed = true
      @mutex = Mutex.new
      if useSessionID and
        if(sessionId.nil?)
          @clientID = 'clientID:Ruby:version=' + getVersion() + ':' + SecureRandom.uuid
        else
          @clientID = 'clientID:Ruby:version=' + getVersion() + ':' + sessionId.to_s
        end

        if(getClientType() != 'RubyPing')
          Thread.new {@clientMonitor.run}
        end
      end

    end


    def getVersion() VERSION end

    def getClientType()
      'Ruby'
    end


    def fixUrlPathPrefix(urlPathPrefix)
      if(urlPathPrefix.nil? or urlPathPrefix == '')
        return ''
      end

      unless(urlPathPrefix.to_s.start_with? '/')
        urlPathPrefix = '/' + urlPathPrefix
      end

      if(urlPathPrefix.to_s.end_with? '/')
        urlPathPrefix = urlPathPrefix[0, urlPathPrefix.length - 1]
      end

      urlPathPrefix

    end

    def execute(method, *params)
      unless (@clientMonitor.isRunning) or (@clientID == nil)
        ExperiLogger.instance.error('The client has already been released. Please create a new client')
        return "ERROR"
      end


      begin
        @mutex.synchronize do
          if @clientID.nil?
            result = @server.call("agent." + method, *params)
          else
            result = @server.call("agent." + method, *(params.insert(0, @clientID)))
          end


          if result == 0
            return "OK"
          end

          @map = result
        end
        unless @map["xpathFound"].nil?
          return activateRecovery( @map["xpathFound"], method, *params)
        end

        logLine = @map['logLine']
        unless logLine.to_s.strip.empty?
          ExperiLogger.instance.info(logLine)
        end


        if (@map.include? 'status') and (not @map['status'])
          if (@throwExceptionOnFailed or ((@map.include? 'throwException') and (not @map['throwException'])))
            ExperiLogger.instance.error('Status: False')
            raise "Exception caught while executing " + method + ": " + @map['errorMessage']
          end
          return @map['errorMessage']
        end

        if (@map.include? 'clientReleased') and (@map['clientReleased'])
          @clientMonitor.release
        end

        return @map

      rescue XMLRPC::FaultException => e
        ExperiLogger.instance.error( e.faultCode + ' ' + e.faultString)
        raise
      end

    end

    def activateRecovery(typeXpathObj, method, *params)
      currentListener = @listeners[typeXpathObj]
      warn "Located : " + typeXpathObj + " Starting recovery process for element"
      delemiterIndex = typeXpathObj.index(':')
      if delemiterIndex >= 0
        currentListener.setClient(self)
      end
      begin
        continueExe = currentListener.recover(typeXpathObj[0,delemiterIndex], typeXpathObj[delemiterIndex, typeXpathObj.size-1])
      rescue NotImplementedError, Exception => e
        if e.is_a?(NotImplementedError)
          ExperiLogger.instance.error('Your Mobile Listener must implement the recover() method')
          raise e
        else
          ExperiLogger.instance.error('Listener Recover Method have thrown an exception with message ' + e.message)
          raise e
        end
      end

      if not continueExe.nil? and continueExe
        ExperiLogger.instance.warn('Successful Recovery attempt, Will try again the original method')
        print "RETRY : method : " + method + "  params "
#           params.collect { |x| print x.to_s " " }

        if (params[0].to_s.include? @clientID)
          params.delete_at(0)
        end
        return execute(method, *params)

      else
        ExperiLogger.instance.error('Unsuccessful Recovery attempt for Mobile Listener : ' + typeXpathObj)
        ExperiLogger.instance.error('Make sure that your recovery method return TRUE if passed')
        return "ERROR"
      end
    end

    def addMobileListener(elementType, xpath, listener)
      unless listener.nil?
        ExperiLogger.instance.info('Adding mobile listener for XPath : ' + xpath + ' From type : ' + elementType)
        if listener.is_a?(Listener)
          key = elementType + ':' + xpath
          listener.setClient(self)
          @listeners[key] = listener
          execute("addXPathListener", elementType, xpath)
        end
      else
        execute("removeXPathListener", elementType, xpath)
        key = elementType + ':' + xpath
        @listeners.delete(key)
        ExperiLogger.instance.info('Removed Listener ' + key)
        puts
      end
    end


    def getLastCommandResultMap()
      return @map
    end

    def pingServer()
      resultMap = execute("pingServer")
    end

    def waitForDevice(query, timeout)["name"]
    begin
      @duration = Time.now.getutc.to_i + timeout/1000.0
      oldThrowExceptionOnFailed = @throwExceptionOnFailed
      @throwExceptionOnFailed = false

      while true
        if Time.now.getutc.to_i > @duration
          ExperiLogger.instance.error('Status: False')
          self.report("Timeout wait for device", false)
          raise "Timeout wait for device"
        end
        result = execute("lockDevice", query)
        @resultMap = result
        if  (@resultMap.include? 'status') and (@resultMap['status'])
          self.report("wait for " + query, true)
          self.report(@resultMap['name'] + " was reserved.", true)
          return @resultMap['name']
        elsif  (@resultMap.include? 'validXPath') and (not @resultMap['validXPath'])
          ExperiLogger.instance.error('XPath is Invalid')
          self.report("waitForDevice - XPath is invalid", false);
          raise "XPath is Invalid"
        elsif  (@resultMap.include? 'license') and (not @resultMap['license'])
          ExperiLogger.instance.error('License is not supported on this agent')
          self.report("waitForDevice - License is not supported on this agent", false)
          raise "License is not supported on this agent"
        end
        Kernel.sleep 2.5
      end
    rescue XMLRPC::FaultException => e
      ExperiLogger.instance.error(e.faultCode + ' ' + e.faultString)
    ensure
      @throwExceptionOnFailed = oldThrowExceptionOnFailed
    end
    end

        VERSION = '11.3'

        def activateVoiceAssistance(text)
            execute("activateVoiceAssistance", text)
        end

        def addDevice(serialNumber, deviceName)
            return execute("addDevice", serialNumber, deviceName)["name"]
        end

        def applicationClearData(packageName)
            execute("applicationClearData", packageName)
        end

        def applicationClose(packageName)
            return execute("applicationClose", packageName)["found"]
        end

        def capture()
            return execute("capture")["outFile"]
        end

        def captureLine(line="Capture")
            return execute("capture", line)["outFile"]
        end

        def captureElement(name, x, y, width, height, similarity=97)
            execute("captureElement", name, x, y, width, height, similarity)
        end

        def clearAllSms()
            execute("clearAllSms")
        end

        def clearDeviceLog()
            execute("clearDeviceLog")
        end

        def clearLocation()
            execute("clearLocation")
        end

        def click(zone, element, index=0, clickCount=1)
            execute("click", zone, element, index, clickCount)
        end

        def clickOffset(zone, element, index=0, clickCount=1, x=0, y=0)
            execute("click", zone, element, index, clickCount, x, y)
        end

        def clickCoordinate(x=0, y=0, clickCount=1)
            execute("clickCoordinate", x, y, clickCount)
        end

        def clickIn(zone, searchElement, index, direction, clickElement, width=0, height=0)
            execute("clickIn", zone, searchElement, index, direction, clickElement, width, height)
        end

        def clickIn2(zone, searchElement, index, direction, clickElementZone, clickElement, width=0, height=0)
            execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, width, height)
        end

        def clickIn2_5(zone, searchElement, index, direction, clickElementZone, clickElement, width=0, height=0, clickCount=1)
            execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, width, height, clickCount)
        end

        def clickIn3(zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex=0, width=0, height=0, clickCount=1)
            execute("clickIn", zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex, width, height, clickCount)
        end

        def clickTableCell(zone, headerElement, headerIndex, rowElement, rowIndex=0)
            execute("clickTableCell", zone, headerElement, headerIndex, rowElement, rowIndex)
        end

        def closeDevice()
            execute("closeDevice")
        end

        def closeDeviceReflection()
            execute("closeDeviceReflection")
        end

        def closeKeyboard()
            execute("closeKeyboard")
        end

        def collectSupportData(zipDestination, applicationPath, device, scenario, expectedResult, actualResult)
            return execute("collectSupportData", zipDestination, applicationPath, device, scenario, expectedResult, actualResult)["source"]
        end

        def collectSupportData2(zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData=true, onlyLatestLogs=true)
            return execute("collectSupportData", zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData, onlyLatestLogs)["source"]
        end

        def deviceAction(action)
            execute("deviceAction", action)
        end

        def drag(zone, element, index=0, xOffset=0, yOffset=0)
            execute("drag", zone, element, index, xOffset, yOffset)
        end

        def dragCoordinates(x1=0, y1=0, x2=0, y2=0)
            execute("dragCoordinates", x1, y1, x2, y2)
        end

        def dragCoordinates2(x1=0, y1=0, x2=0, y2=0, time=2000)
            execute("dragCoordinates", x1, y1, x2, y2, time)
        end

        def dragDrop(zone, dragElement, dragIndex, dropElement, dropIndex=0)
            execute("dragDrop", zone, dragElement, dragIndex, dropElement, dropIndex)
        end

        def drop()
            execute("drop")
        end

        def elementGetProperty(zone, element, index, property)
            return execute("elementGetProperty", zone, element, index, property)["text"]
        end

        def elementGetTableRowsCount2(tableLocator, tableIndex=0, visible=false)
            return execute("elementGetTableRowsCount", tableLocator, tableIndex, visible)["count"]
        end

        def elementGetTableRowsCount(zone, tableLocator, tableIndex=0, visible=false)
            return execute("elementGetTableRowsCount", zone, tableLocator, tableIndex, visible)["count"]
        end

        def elementGetTableValue(rowLocator, rowLocatorIndex, columnLocator)
            return execute("elementGetTableValue", rowLocator, rowLocatorIndex, columnLocator)["text"]
        end

        def elementGetText(zone, element, index=0)
            return execute("elementGetText", zone, element, index)["text"]
        end

        def elementListPick(listZone, listLocator, elementZone, elementLocator, index=0, click=true)
            execute("elementListPick", listZone, listLocator, elementZone, elementLocator, index, click)
        end

        def elementListSelect(listLocator, elementLocator, index=0, click=true)
            execute("elementListSelect", listLocator, elementLocator, index, click)
        end

        def elementListVisible(listLocator, elementLocator, index=0)
            return execute("elementListVisible", listLocator, elementLocator, index)["found"]
        end

        def elementScrollToTableRow2(tableLocator, tableIndex=0, rowIndex=0)
            execute("elementScrollToTableRow", tableLocator, tableIndex, rowIndex)
        end

        def elementScrollToTableRow(zone, tableLocator, tableIndex=0, rowIndex=0)
            execute("elementScrollToTableRow", zone, tableLocator, tableIndex, rowIndex)
        end

        def elementSendText(zone, element, index, text)
            execute("elementSendText", zone, element, index, text)
        end

        def elementSetProperty(zone, element, index, property, value)
            return execute("elementSetProperty", zone, element, index, property, value)["text"]
        end

        def elementSwipe(zone, element, index, direction, offset=0, time=2000)
            execute("elementSwipe", zone, element, index, direction, offset, time)
        end

        def elementSwipeWhileNotFound(componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex=0, delay=1000, rounds=5, click=true)
            return execute("elementSwipeWhileNotFound", componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex, delay, rounds, click)["found"]
        end

        def endTransaction(name)
            execute("endTransaction", name)
        end

        def exit()
            execute("exit")
        end

        def extractLanguageFiles(application, directoryPath, allowOverwrite=true)
            execute("extractLanguageFiles", application, directoryPath, allowOverwrite)
        end

        def findElements(zone, parent, by, value)
            return execute("findElements", zone, parent, by, value)["textArray"]
        end

        def flick(direction, offset=0)
            execute("flick", direction, offset)
        end

        def flickCoordinate(x, y, direction)
            execute("flickCoordinate", x, y, direction)
        end

        def flickElement(zone, element, index, direction)
            execute("flickElement", zone, element, index, direction)
        end

        def forceTouch(zone, element, index=0, duration=100, force=100, dragDistanceX=0, dragDistanceY=0, dragDuration=1500)
            execute("forceTouch", zone, element, index, duration, force, dragDistanceX, dragDistanceY, dragDuration)
        end

        def generateReport()
            return execute("generateReport")["text"]
        end

        def generateReport2(releaseClient=true)
            return execute("generateReport", releaseClient)["text"]
        end

        def generateReport(releaseClient, propFilePath)
            return execute("generateReport", releaseClient, propFilePath)["text"]
        end

        def getAllSms(timeout=5000)
            return execute("getAllSms", timeout)["textArray"]
        end

        def getAllValues(zone, element, property)
            return execute("getAllValues", zone, element, property)["textArray"]
        end

        def getAllZonesWithElement(element)
            return execute("getAllZonesWithElement", element)["text"]
        end

        def getAvailableAgentPort()
            return execute("getAvailableAgentPort")["port"]
        end

        def getAvailableAgentPort2(featureName)
            return execute("getAvailableAgentPort", featureName)["port"]
        end

        def getBrowserTabIdList()
            return execute("getBrowserTabIdList")["textArray"]
        end

        def getConnectedDevices()
            return execute("getConnectedDevices")["text"]
        end

        def getContextList()
            return execute("getContextList")["textArray"]
        end

        def getCoordinateColor(x=0, y=0)
            return execute("getCoordinateColor", x, y)["color"]
        end

        def getCounter(counterName)
            return execute("getCounter", counterName)["text"]
        end

        def getCurrentApplicationName()
            return execute("getCurrentApplicationName")["text"]
        end

        def getCurrentBrowserTabId()
            return execute("getCurrentBrowserTabId")["text"]
        end

        def getDefaultTimeout()
            return execute("getDefaultTimeout")["timeout"]
        end

        def getDeviceLog()
            return execute("getDeviceLog")["path"]
        end

        def getDeviceProperty(key)
            return execute("getDeviceProperty", key)["text"]
        end

        def getDevicesInformation()
            return execute("getDevicesInformation")["text"]
        end

        def getElementCount(zone, element)
            return execute("getElementCount", zone, element)["count"]
        end

        def getElementCountIn(zoneName, elementSearch, index, direction, elementCountZone, elementCount, width=0, height=0)
            return execute("getElementCountIn", zoneName, elementSearch, index, direction, elementCountZone, elementCount, width, height)["count"]
        end

        def getInstalledApplications()
            return execute("getInstalledApplications")["text"]
        end

        def getLastSMS(timeout=5000)
            return execute("getLastSMS", timeout)["text"]
        end

        def getMonitorsData(cSVfilepath)
            return execute("getMonitorsData", cSVfilepath)["text"]
        end

        def getNetworkConnection(connection)
            return execute("getNetworkConnection", connection)["found"]
        end

        def getNVProfiles()
            return execute("getNVProfiles")["textArray"]
        end

        def getPickerValues(zone, pickerElement, index=0, wheelIndex=0)
            return execute("getPickerValues", zone, pickerElement, index, wheelIndex)["textArray"]
        end

        def getPosition(zone, element)
            return execute("getPosition", zone, element)["click"]
        end

        def getPositionWindowRelative(zone, element)
            return execute("getPositionWindowRelative", zone, element)["centerWinRelative"]
        end

        def getProperty(property)
            return execute("getProperty", property)["text"]
        end

        def getSimCard()
            return execute("getSimCard")["text"]
        end

        def getSimCards(readyToUse=true)
            return execute("getSimCards", readyToUse)["textArray"]
        end

        def getTableCellText(zone, headerElement, headerIndex, rowElement, rowIndex=0, width=0, height=0)
            return execute("getTableCellText", zone, headerElement, headerIndex, rowElement, rowIndex, width, height)["text"]
        end

        def getText(zone)
            return execute("getText", zone)["text"]
        end

        def getTextIn(zone, element, index, direction, width=0, height=0)
            return execute("getTextIn", zone, element, index, direction, width, height)["text"]
        end

        def getTextIn2(zone, element, index, textZone, direction, width=0, height=0)
            return execute("getTextIn", zone, element, index, textZone, direction, width, height)["text"]
        end

        def getTextIn3(zone, element, index, textZone, direction, width=0, height=0, xOffset=0, yOffset=0)
            return execute("getTextIn", zone, element, index, textZone, direction, width, height, xOffset, yOffset)["text"]
        end

        def getVisualDump(type="Native")
            return execute("getVisualDump", type)["text"]
        end

        def hybridClearCache(clearCookies=true, clearCache=true)
            resultMap = execute("hybridClearCache", clearCookies, clearCache)
        end

        def hybridGetHtml(webViewLocator, index=0)
            return execute("hybridGetHtml", webViewLocator, index)["text"]
        end

        def hybridRunJavascript(webViewLocator, index, script)
            return execute("hybridRunJavascript", webViewLocator, index, script)["text"]
        end

        def hybridSelect(webViewLocator, index, method, value, select)
            execute("hybridSelect", webViewLocator, index, method, value, select)
        end

        def hybridWaitForPageLoad(timeout=10000)
            execute("hybridWaitForPageLoad", timeout)
        end

        def install(path, sign=false)
            return execute("install", path, sign)["found"]
        end

        def install2(path, instrument=true, keepData=false)
            return execute("install", path, instrument, keepData)["found"]
        end

        def installWithCustomKeystore(path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword)
            return execute("installWithCustomKeystore", path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword)["found"]
        end

        def isElementBlank(zone, element, index=0, colorGroups=10)
            return execute("isElementBlank", zone, element, index, colorGroups)["found"]
        end

        def isElementFound2(zone, element)
            return execute("isElementFound", zone, element)["found"]
        end

        def isElementFound(zone, element, index=0)
            return execute("isElementFound", zone, element, index)["found"]
        end

        def isFoundIn(zone, searchElement, index, direction, elementFindZone, elementToFind, width=0, height=0)
            return execute("isFoundIn", zone, searchElement, index, direction, elementFindZone, elementToFind, width, height)["found"]
        end

        def isTextFound(zone, element, ignoreCase)
            return execute("isTextFound", zone, element, ignoreCase)["found"]
        end

        def launch2(applicationIdentifier, launchOptions)
            execute("launch", applicationIdentifier, launchOptions)
        end

        def launch(activityURL, instrument=true, stopIfRunning=false)
            execute("launch", activityURL, instrument, stopIfRunning)
        end

        def listSelect(sendRest, sendNavigation, delay, textToIdentify, color, rounds=5, sendonfind="{ENTER}")
            return execute("listSelect", sendRest, sendNavigation, delay, textToIdentify, color, rounds, sendonfind)["found"]
        end

        def longClick(zone, element, index=0, clickCount=1, x=0, y=0)
            execute("longClick", zone, element, index, clickCount, x, y)
        end

        def maximize()
            execute("maximize")
        end

        def multiClick(zone, element, index, fingerIndex)
            execute("multiClick", zone, element, index, fingerIndex)
        end

        def multiClickCoordinate(x, y, fingerIndex)
            execute("multiClickCoordinate", x, y, fingerIndex)
        end

        def multiClickOffset(zone, element, index, x, y, fingerIndex)
            execute("multiClickOffset", zone, element, index, x, y, fingerIndex)
        end

        def multiSwipe(direction, offset, time, fingerIndex)
            execute("multiSwipe", direction, offset, time, fingerIndex)
        end

        def multiTouchDown(zone, element, index, fingerIndex)
            execute("multiTouchDown", zone, element, index, fingerIndex)
        end

        def multiTouchDownCoordinate(x, y, fingerIndex)
            execute("multiTouchDownCoordinate", x, y, fingerIndex)
        end

        def multiTouchMove(zone, element, index, fingerIndex)
            execute("multiTouchMove", zone, element, index, fingerIndex)
        end

        def multiTouchMoveCoordinate(x, y, fingerIndex)
            execute("multiTouchMoveCoordinate", x, y, fingerIndex)
        end

        def multiTouchUp(fingerIndex)
            execute("multiTouchUp", fingerIndex)
        end

        def multiWait(time, fingerIndex)
            execute("multiWait", time, fingerIndex)
        end

        def openDevice()
            execute("openDevice")
        end

        def p2cx(percentage=0)
            return execute("p2cx", percentage)["pixel"]
        end

        def p2cy(percentage=0)
            return execute("p2cy", percentage)["pixel"]
        end

        def performMultiGesture()
            execute("performMultiGesture")
        end

        def pinch(inside=true, x=0, y=0, radius=100)
            return execute("pinch", inside, x, y, radius)["found"]
        end

        def pinch2(inside=true, x=0, y=0, radius=100, horizontal=false)
            return execute("pinch", inside, x, y, radius, horizontal)["found"]
        end

        def portForward(localPort, remotePort)
            return execute("portForward", localPort, remotePort)["port"]
        end

        def pressWhileNotFound(zone, elementtoclick, elementtofind, timeout=10000, delay=0)
            execute("pressWhileNotFound", zone, elementtoclick, elementtofind, timeout, delay)
        end

        def pressWhileNotFound2(zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex=0, timeout=10000, delay=0)
            execute("pressWhileNotFound", zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex, timeout, delay)
        end

        def reboot(timeout=120000)
            return execute("reboot", timeout)["status"]
        end

        def receiveIncomingCall(fromNumber, hangupInSeconds)
            execute("receiveIncomingCall", fromNumber, hangupInSeconds)
        end

        def receiveIncomingSMS(fromNumber, msg)
            execute("receiveIncomingSMS", fromNumber, msg)
        end

        def releaseClient()
            execute("releaseClient")
        end

        def releaseDevice(deviceName, releaseAgent=true, removeFromDeviceList=false, releaseFromCloud=true)
            execute("releaseDevice", deviceName, releaseAgent, removeFromDeviceList, releaseFromCloud)
        end

        def report(message, status)
            execute("report", message, status)
        end

        def reportWithImage(pathToImage, message, status)
            execute("report", pathToImage, message, status)
        end

        def resetDeviceBridge()
            execute("resetDeviceBridge")
        end

        def resetDeviceBridgeOS(deviceType)
            execute("resetDeviceBridge", deviceType)
        end

        def rightClick(zone, element, index=0, clickCount=1, x=0, y=0)
            execute("rightClick", zone, element, index, clickCount, x, y)
        end

        def run(command)
            return execute("run", command)["text"]
        end

        def LayoutTest(xml)
            return execute("runLayoutTest", xml)["text"]
        end

        def runNativeAPICall(zone, element, index, script)
            return execute("runNativeAPICall", zone, element, index, script)["text"]
        end

        def sendText(text)
            execute("sendText", text)
        end

        def sendWhileNotFound(toSend, zone, elementtofind, timeout=10000, delay=1000)
            execute("sendWhileNotFound", toSend, zone, elementtofind, timeout, delay)
        end

        def sendWhileNotFound2(toSend, zone, elementtofind, elementtofindindex=0, timeout=10000, delay=1000)
            execute("sendWhileNotFound", toSend, zone, elementtofind, elementtofindindex, timeout, delay)
        end

        def setApplicationTitle(title)
            execute("setApplicationTitle", title)
        end

        def setAuthenticationReply(reply="Success", delay=0)
            execute("setAuthenticationReply", reply, delay)
        end

        def setContext(context="NATIVE_APP")
            execute("setContext", context)
        end

        def setDefaultClickDownTime(downTime=100)
            execute("setDefaultClickDownTime", downTime)
        end

        def setDefaultTimeout(newTimeout=20000)
            return execute("setDefaultTimeout", newTimeout)["timeout"]
        end

        def setDefaultWebView(webViewLocator)
            execute("setDefaultWebView", webViewLocator)
        end

        def setDevice(device)
            execute("setDevice", device)
        end

        def setDragStartDelay(delay=0)
            execute("setDragStartDelay", delay)
        end

        def setInKeyDelay(delay=50)
            execute("setInKeyDelay", delay)
        end

        def setKeyToKeyDelay(delay=50)
            execute("setKeyToKeyDelay", delay)
        end

        def setLanguage(language)
            execute("setLanguage", language)
        end

        def setLanguagePropertiesFile(propertiesfile)
            execute("setLanguagePropertiesFile", propertiesfile)
        end

        def setLocation(latitude="0.0", longitude="0.0")
            execute("setLocation", latitude, longitude)
        end

        def setMonitorPollingInterval(timemilli=30000)
            execute("setMonitorPollingInterval", timemilli)
        end

        def setMonitorTestState(testStatus)
            execute("setMonitorTestState", testStatus)
        end

        def setNetworkConditions(profile)
            execute("setNetworkConditions", profile)
        end

        def setNetworkConditions2(profile, duration=0)
            execute("setNetworkConditions", profile, duration)
        end

        def setNetworkConnection(connection, enable)
            execute("setNetworkConnection", connection, enable)
        end

        def setOcrIgnoreCase(ignoreCase)
            execute("setOcrIgnoreCase", ignoreCase)
        end

        def setOcrTrainingFilePath(trainingPath)
            execute("setOcrTrainingFilePath", trainingPath)
        end

        def setPickerValues(zoneName, elementName, index, wheelIndex, value)
            return execute("setPickerValues", zoneName, elementName, index, wheelIndex, value)["text"]
        end

        def setProjectBaseDirectory(projectBaseDirectory)
            execute("setProjectBaseDirectory", projectBaseDirectory)
        end

        def setProperty(key, value)
            execute("setProperty", key, value)
        end

        def setRedToBlue(redToBlue)
            execute("setRedToBlue", redToBlue)
        end

        def setReporter(reporterName, directory)
            return execute("setReporter", reporterName, directory)["text"]
        end

        def setReporter2(reporterName, directory, testName)
            return execute("setReporter", reporterName, directory, testName)["text"]
        end

        def setShowImageAsLink(showImageAsLink)
            execute("setShowImageAsLink", showImageAsLink)
        end

        def setShowImageInReport(showImageInReport=true)
            execute("setShowImageInReport", showImageInReport)
        end

        def setShowPassImageInReport(showPassImageInReport=true)
            execute("setShowPassImageInReport", showPassImageInReport)
        end

        def setShowReport(showReport=true)
            execute("setShowReport", showReport)
        end

        def setSimCard(simCardName)
            execute("setSimCard", simCardName)
        end

        def setSpeed(speed)
            execute("setSpeed", speed)
        end

        def setTestStatus(status, message)
            execute("setTestStatus", status, message)
        end

        def setWebAutoScroll(autoScroll=true)
            execute("setWebAutoScroll", autoScroll)
        end

        def setWindowSize(width=0, height=0)
            execute("setWindowSize", width, height)
        end

        def shake()
            execute("shake")
        end

        def simulateCapture(picturePath)
            execute("simulateCapture", picturePath)
        end

        def sleep(time=1000)
            execute("sleep", time)
        end

        def startAudioPlay(audioFile)
            execute("startAudioPlay", audioFile)
        end

        def startAudioRecording(audioFile)
            execute("startAudioRecording", audioFile)
        end

        def startCall(skypeUser, skypePassword, number, duration=0)
            execute("startCall", skypeUser, skypePassword, number, duration)
        end

        def startLoggingDevice(path)
            execute("startLoggingDevice", path)
        end

        def startMonitor(packageName)
            execute("startMonitor", packageName)
        end

        def startMultiGesture(name)
            execute("startMultiGesture", name)
        end

        def startStepsGroup(caption)
            execute("startStepsGroup", caption)
        end

        def startTransaction(name)
            execute("startTransaction", name)
        end

        def startVideoRecord()
            execute("startVideoRecord")
        end

        def stopAudioPlay()
            execute("stopAudioPlay")
        end

        def stopAudioRecording()
            execute("stopAudioRecording")
        end

        def stopLoggingDevice()
            return execute("stopLoggingDevice")["text"]
        end

        def stopStepsGroup()
            execute("stopStepsGroup")
        end

        def stopVideoRecord()
            return execute("stopVideoRecord")["text"]
        end

        def swipe(direction, offset=0)
            execute("swipe", direction, offset)
        end

        def swipe2(direction, offset=0, time=500)
            execute("swipe", direction, offset, time)
        end

        def swipeWhileNotFound(direction, offset, zone, elementtofind, delay=1000, rounds=5, click=true)
            return execute("swipeWhileNotFound", direction, offset, zone, elementtofind, delay, rounds, click)["found"]
        end

        def swipeWhileNotFound3(direction, offset, swipeTime, zone, elementtofind, delay=1000, rounds=5, click=true)
            return execute("swipeWhileNotFound", direction, offset, swipeTime, zone, elementtofind, delay, rounds, click)["found"]
        end

        def swipeWhileNotFound2(direction, offset, swipeTime, zone, elementtofind, elementtofindindex=0, delay=1000, rounds=5, click=true)
            return execute("swipeWhileNotFound", direction, offset, swipeTime, zone, elementtofind, elementtofindindex, delay, rounds, click)["found"]
        end

        def switchToBrowserTab(tabId)
            execute("switchToBrowserTab", tabId)
        end

        def sync(silentTime=2000, sensitivity=0, timeout=10000)
            return execute("sync", silentTime, sensitivity, timeout)["found"]
        end

        def syncElements(silentTime=2000, timeout=10000)
            return execute("syncElements", silentTime, timeout)["found"]
        end

        def textFilter(color, sensitivity=15)
            execute("textFilter", color, sensitivity)
        end

        def touchDown(zone, element, index=0)
            execute("touchDown", zone, element, index)
        end

        def touchDownCoordinate(x=0, y=0)
            execute("touchDownCoordinate", x, y)
        end

        def touchMove(zone, element, index=0)
            execute("touchMove", zone, element, index)
        end

        def touchMoveCoordinate(x=0, y=0)
            execute("touchMoveCoordinate", x, y)
        end

        def touchUp()
            execute("touchUp")
        end

        def uninstall(application)
            return execute("uninstall", application)["found"]
        end

        def verifyElementFound(zone, element, index=0)
            resultMap = execute("verifyElementFound", zone, element, index)
        end

        def verifyElementNotFound(zone, element, index=0)
            resultMap = execute("verifyElementNotFound", zone, element, index)
        end

        def verifyIn(zone, searchElement, index, direction, elementFindZone, elementToFind, width=0, height=0)
            resultMap = execute("verifyIn", zone, searchElement, index, direction, elementFindZone, elementToFind, width, height)
        end

        def waitForAudioPlayEnd(timeout)
            execute("waitForAudioPlayEnd", timeout)
        end

        def waitForElement(zone, element, index=0, timeout=10000)
            return execute("waitForElement", zone, element, index, timeout)["found"]
        end

        def waitForElementToVanish(zone, element, index=0, timeout=10000)
            return execute("waitForElementToVanish", zone, element, index, timeout)["found"]
        end

        def waitForWindow(name, timeout=10000)
            return execute("waitForWindow", name, timeout)["found"]
        end




  end

  class ExecutionClient < Client

    def initialize(gridClient, ehmHostOrIp, ehmPort, secured, sessionId, urlPathPrefix)
      super(ehmHostOrIp, ehmPort, true, secured, sessionId, urlPathPrefix)

      @gridClient = gridClient
      @sessionId = sessionId

    end

    #
    #  Lock Pre Reserved Execution Device
    #  @param timeoutInMilliseconds - Timeout in milliseconds
    #
    def lockPreReservedDevice(timeoutInMilliseconds)
      execute('lockPreReservedDevice', timeoutInMilliseconds)
    end

    def generateReport2(releaseClient=true)
      result = super(releaseClient)

      if (releaseClient && !@gridClient.nil?)
        ExperiLogger.instance.info('The method releaseClient try to releaseDeviceFromExecution')
        @gridClient.releaseDeviceFromExecution(self)
      end

      result
    end

    def generateReport(releaseClient, propFilePath)
      result = super(releaseClient, propFilePath)

     if (releaseClient && !@gridClient.nil?)
        ExperiLogger.instance.info('The method releaseClient try to releaseDeviceFromExecution')
        @gridClient.releaseDeviceFromExecution(self)
      end

      result
    end

    def releaseClient()
      super

      unless (@gridClient.nil?)
          ExperiLogger.instance.info('The method releaseClient try to releaseDeviceFromExecution')
          @gridClient.releaseDeviceFromExecution(self)
       else
         ExperiLogger.instance.error('The method releaseClient failed to releaseDeviceFromExecution, gridClient is null')
      end
    end

    def getSessionId()
      @sessionId
    end

    def setDevice(device)
     throw "The method 'setDevice' could not be called from Grid Execution Client"
    end

    def waitForDevice(query, timeout)["name"]
      throw "The method 'waitForDevice' could not be called from Grid Execution Client"
    end

    def addDevice(serialNumber, deviceName)
      throw "The method 'addDevice' could not be called from Grid Execution Client"
    end

    def getDevicesInformation()
      throw "The method 'getDevicesInformation' could not be called from Grid Execution Client"
    end

    def startVideoRecord()
      ExperiLogger.instance.error("The method 'startVideoRecord' could not be called from Grid Execution Client")
    end

    def stopVideoRecord()
      ExperiLogger.instance.error("The method 'stopVideoRecord' could not be called from Grid Execution Client")
    end

  end

  class PingRubyClient < Client

    def initialize(host, port, secured, sessionId, urlPrefix)
      super(host, port, secured)
    end

    def pingServer()
      begin
        super
      rescue Exception => e
        raise 'Connectivity Exception, Failed to connect to External Host'
      end
    end

    protected

    def getClientType()
      'RubyPing'
    end
  end

  require 'singleton'
  require 'logger'
  class ExperiLogger < Logger
    include Singleton

    def initialize()
      super($stdout)
      @formatter = ExperiFormater.new
    end


    def warn(message)
      add(Logger::WARN, message)
    end

    def debug(message)
      add(Logger::DEBUG, message)
    end

    def info(message)
      add(Logger::INFO, message)
    end

    def error(message)
      add(Logger::ERROR, message)
    end

    class ExperiFormater < Formatter
      Format = "%s[%s] %s : %s\n".freeze

      attr_accessor :datetime_format

      def initialize
        @datetime_format = nil
      end

      def call(severity, time, progname, msg)
        Format % [format_datetime(time), Thread.current.to_s[9..17] ,severity, msg2str(msg)]
      end

      private

      def format_datetime(time)
        time.strftime(@datetime_format || "%Y-%m-%d %H:%M:%S.%3N ".freeze)
      end

      def msg2str(msg)
        case msg
          when ::String
            msg
          when ::Exception
            "#{ msg.message } (#{ msg.class })\n" <<
                (msg.backtrace || []).join("\n")
          else
            msg.inspect
        end
      end
    end
  end
end
