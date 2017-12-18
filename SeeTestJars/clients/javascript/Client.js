var MobileClient = function() {

      var xmlrpc = require('xmlrpc');
      var crypto = require('crypto');
      var client = null;
      var map = null;
      var clientID = null;
      var url = null;
      var timeout = 120000;
	  var host = null;
	  var port = null;
	  var debug = false;
	  var keepAliveTimer = null;
      var lastExecuteOperationTimestampInMsec = 0;

	  function setDebug(debugFlag) {
		  debug = debugFlag;
	  }

	  function _log(str) {
			console.log(new Date().toISOString().replace(/T/, ' ')+":"+str);
	  }

	  function sendKeepAliveMsg() {
          var currentTimestampInMsec = Date.now();
          if (lastExecuteOperationTimestampInMsec > 0 &&
              currentTimestampInMsec - lastExecuteOperationTimestampInMsec > 60000) {
            _log("Warning: No client operation has been called for more than one minute. You should release the client in order to end the program");
          }
          pingServer();
      }

	  function initialize(initHost, initPort, useSessionID) {
			if (!initHost)
	            initHost = "127.0.0.1";
	        if (!initPort)
	            initPort = 8889;
	        if (!useSessionID)
	            useSessionID = true;
			host = initHost;
			port = initPort;

	        client = xmlrpc.createClient({ host: host, port: port , path: '/xmlrpc'});

			url = "http://" + host + ":" + port + "/xmlrpc";
			_log("initialize with url="+url);

            if (useSessionID) {
                clientID = "clientID:JS:version=" + getVersion() + ":" + crypto.randomBytes(64).toString('hex');
   				keepAliveTimer = setInterval(sendKeepAliveMsg, 5000);
            }
	    }

		function throwException(errorMsg) {
			throw errorMsg;
		}

        function performExecuteAsync(method, params, retAttribute, callback) {
            client.methodCall("agent." + method, params, function(error, data) {
                if (error) {
                    _log("Error:");
                    _log(error.res);
                    throwException("Error during XML-RPC call");
                } else {
                    if (!data)
                        callback("OK");
                    map = data;
                    if (map.hasOwnProperty("status") && map.status === false) {
                        _log("Status: False");
						if (map.hasOwnProperty("throwException") && map.throwException === true) {
							throwException("Exception caught while executing " + method + ": " + map.errorMessage);
						}
						callback(map["errorMessage"]);
                    }
					if (map.hasOwnProperty("logLine")) {
                        _log(map["logLine"]);
                    }
                    if (retAttribute)
                        callback(map[retAttribute]);
                    else
                        callback(map);
                }
            });
        }

        // execute XML-RPC method
        // Parameters:
        // method - String with method names
        // params - Array of XML-RPC parameters
        // retAttribute - if null means what value from the XML-RPC result map to return
        // callback - if null call will be synchronous
		function execute(method, params, retAttribute, callback) {

			if (debug)
				_log(clientID+":"+" Method: "+method+" Params:"+params);

            if (method !== "pingserver")
				lastExecuteOperationTimestampInMsec = Date.now();

			if (method === "releaseClient" || (method === "generateReport" && params.length > 0 && params[0] === true) )
				clearInterval(keepAliveTimer);

		    // duplicate params given by user
            var newParams = params.slice(0);
			if (clientID)
                newParams.splice(0, 0, clientID);
            var responseRecieved = false;
            var result = null;

            // If user provided callback than execute will ne performed asynchronously
            if (callback)
                performExecuteAsync(method, newParams, retAttribute, callback);
            else { // No callback means user wants to execute method synchronously
                // Wait no more than timeout milliseconds

                var myTimeout = setTimeout(function() {
                    responseRecieved = true;
					throwException("XML-RPC timeout during call to method="+method+" with params="+params);
                }, timeout);

                // execute method
                performExecuteAsync(method, newParams, retAttribute, function(response) {
                    result = response;
                    clearTimeout(myTimeout);
                    responseRecieved = true;
                });

                // wait until either response is received or timeout expires
                while(!responseRecieved) {
                      require('deasync').runLoopOnce();
                }

                return result;
            }
       }

		function getLastCommandResultMap() {
			return map;
		}

        function pingServer(callback) {
            return execute("pingserver", [], null, callback);
        }

        function waitForDevice(query, waitForDeviceTimeout, callback) {
			if (!timeout) {
				throwException("no timeout parameter given for setDevice");
			}

			_log("Experitest JS Client:waitForDevice query="+query);

			var result = null;
			var responseRecieved = false;
			var waitForDeviceTimeoutHasExpired = false;

			var myTimeout = setTimeout(function() {
                waitForDeviceTimeoutHasExpired = true;
            }, waitForDeviceTimeout);

			while (!responseRecieved && !waitForDeviceTimeoutHasExpired) {
				var reqTimeoutHasExpired = false;
				var params = [];
				if (clientID)
					params = [clientID, query];
				else
					params = [query];

				var reqTimeout = setTimeout(function() {
					reqTimeoutHasExpired = true;
				}, waitForDeviceTimeout);

				// execute method
				performExecuteAsync("lockDevice", params, null, function(response) {
					result = response;
					clearTimeout(reqTimeout);
					responseRecieved = true;
				});

				// wait until either response is received or timeout expires
				while(!responseRecieved) {
					  require('deasync').runLoopOnce();
				}

				if (result.hasOwnProperty("status") && result.status === true) {
					clearTimeout(myTimeout);
					report("wait for " + query, true);
					report(result.name + " was reserved.", true);
					_log("waitForDevice("+query+") completed successfully and "+result.name+" was reserved");
					return result.name;

				} else if (result.hasOwnProperty("validXPath") && result.validXPath === false) {
					_log("XPath is Invalid");
					report("waitForDevice - XPath is invalid", false);
					throwException("XPath is Invalid");
				} else if (result.hasOwnProperty("license") && result.license === false) {
					_log("XPath is Invalid");
					report("waitForDevice - XPath is invalid", false);
					throwException("XPath is Invalid");
				}

				responseRecieved = false;
				sleep(2500);
			}

			if (waitForDeviceTimeoutHasExpired) {
				_log("Status: False");
				report("Timeout wait for device", false);
				throwException("Timeout wait for device");
			}


        }

		function getVersion() { return "11.3"; }

		function activateVoiceAssistance(text, callback) {
			execute("activateVoiceAssistance", [text], null, callback);
		}

		function addDevice(serialNumber, deviceName, callback) {
			return execute("addDevice", [serialNumber, deviceName], "name", callback);
		}

		function applicationClearData(packageName, callback) {
			execute("applicationClearData", [packageName], null, callback);
		}

		function applicationClose(packageName, callback) {
			return execute("applicationClose", [packageName], "found", callback);
		}

		function capture(callback) {
			return execute("capture", [], "outFile", callback);
		}

		function captureLine(line, callback) {
			if (line === undefined) 
				line = Capture;
			return execute("capture", [line], "outFile", callback);
		}

		function captureElement(name, x, y, width, height, similarity, callback) {
			if (similarity === undefined) 
				similarity = 97;
			execute("captureElement", [name, x, y, width, height, similarity], null, callback);
		}

		function clearAllSms(callback) {
			execute("clearAllSms", [], null, callback);
		}

		function clearDeviceLog(callback) {
			execute("clearDeviceLog", [], null, callback);
		}

		function clearLocation(callback) {
			execute("clearLocation", [], null, callback);
		}

		function click(zone, element, index, clickCount, callback) {
			if (index === undefined) 
				index = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			execute("click", [zone, element, index, clickCount], null, callback);
		}

		function clickOffset(zone, element, index, clickCount, x, y, callback) {
			if (index === undefined) 
				index = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("click", [zone, element, index, clickCount, x, y], null, callback);
		}

		function clickCoordinate(x, y, clickCount, callback) {
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			execute("clickCoordinate", [x, y, clickCount], null, callback);
		}

		function clickIn(zone, searchElement, index, direction, clickElement, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			execute("clickIn", [zone, searchElement, index, direction, clickElement, width, height], null, callback);
		}

		function clickIn2(zone, searchElement, index, direction, clickElementZone, clickElement, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			execute("clickIn", [zone, searchElement, index, direction, clickElementZone, clickElement, width, height], null, callback);
		}

		function clickIn2_5(zone, searchElement, index, direction, clickElementZone, clickElement, width, height, clickCount, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			execute("clickIn", [zone, searchElement, index, direction, clickElementZone, clickElement, width, height, clickCount], null, callback);
		}

		function clickIn3(zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex, width, height, clickCount, callback) {
			if (index === undefined) 
				index = 0;
			if (clickElementIndex === undefined) 
				clickElementIndex = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			execute("clickIn", [zone, searchElement, index, direction, clickElementZone, clickElement, clickElementIndex, width, height, clickCount], null, callback);
		}

		function clickTableCell(zone, headerElement, headerIndex, rowElement, rowIndex, callback) {
			if (headerIndex === undefined) 
				headerIndex = 0;
			if (rowIndex === undefined) 
				rowIndex = 0;
			execute("clickTableCell", [zone, headerElement, headerIndex, rowElement, rowIndex], null, callback);
		}

		function closeDevice(callback) {
			execute("closeDevice", [], null, callback);
		}

		function closeDeviceReflection(callback) {
			execute("closeDeviceReflection", [], null, callback);
		}

		function closeKeyboard(callback) {
			execute("closeKeyboard", [], null, callback);
		}

		function collectSupportData(zipDestination, applicationPath, device, scenario, expectedResult, actualResult, callback) {
			return execute("collectSupportData", [zipDestination, applicationPath, device, scenario, expectedResult, actualResult], "source", callback);
		}

		function collectSupportData2(zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData, onlyLatestLogs, callback) {
			if (withCloudData === undefined) 
				withCloudData = true;
			if (onlyLatestLogs === undefined) 
				onlyLatestLogs = true;
			return execute("collectSupportData", [zipDestination, applicationPath, device, scenario, expectedResult, actualResult, withCloudData, onlyLatestLogs], "source", callback);
		}

		function deviceAction(action, callback) {
			execute("deviceAction", [action], null, callback);
		}

		function drag(zone, element, index, xOffset, yOffset, callback) {
			if (index === undefined) 
				index = 0;
			if (xOffset === undefined) 
				xOffset = 0;
			if (yOffset === undefined) 
				yOffset = 0;
			execute("drag", [zone, element, index, xOffset, yOffset], null, callback);
		}

		function dragCoordinates(x1, y1, x2, y2, callback) {
			if (x1 === undefined) 
				x1 = 0;
			if (y1 === undefined) 
				y1 = 0;
			if (x2 === undefined) 
				x2 = 0;
			if (y2 === undefined) 
				y2 = 0;
			execute("dragCoordinates", [x1, y1, x2, y2], null, callback);
		}

		function dragCoordinates2(x1, y1, x2, y2, time, callback) {
			if (x1 === undefined) 
				x1 = 0;
			if (y1 === undefined) 
				y1 = 0;
			if (x2 === undefined) 
				x2 = 0;
			if (y2 === undefined) 
				y2 = 0;
			if (time === undefined) 
				time = 2000;
			execute("dragCoordinates", [x1, y1, x2, y2, time], null, callback);
		}

		function dragDrop(zone, dragElement, dragIndex, dropElement, dropIndex, callback) {
			if (dragIndex === undefined) 
				dragIndex = 0;
			if (dropIndex === undefined) 
				dropIndex = 0;
			execute("dragDrop", [zone, dragElement, dragIndex, dropElement, dropIndex], null, callback);
		}

		function drop(callback) {
			execute("drop", [], null, callback);
		}

		function elementGetProperty(zone, element, index, property, callback) {
			if (index === undefined) 
				index = 0;
			return execute("elementGetProperty", [zone, element, index, property], "text", callback);
		}

		function elementGetTableRowsCount2(tableLocator, tableIndex, visible, callback) {
			if (tableIndex === undefined) 
				tableIndex = 0;
			if (visible === undefined) 
				visible = false;
			return execute("elementGetTableRowsCount", [tableLocator, tableIndex, visible], "count", callback);
		}

		function elementGetTableRowsCount(zone, tableLocator, tableIndex, visible, callback) {
			if (tableIndex === undefined) 
				tableIndex = 0;
			if (visible === undefined) 
				visible = false;
			return execute("elementGetTableRowsCount", [zone, tableLocator, tableIndex, visible], "count", callback);
		}

		function elementGetTableValue(rowLocator, rowLocatorIndex, columnLocator, callback) {
			if (rowLocatorIndex === undefined) 
				rowLocatorIndex = 0;
			return execute("elementGetTableValue", [rowLocator, rowLocatorIndex, columnLocator], "text", callback);
		}

		function elementGetText(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			return execute("elementGetText", [zone, element, index], "text", callback);
		}

		function elementListPick(listZone, listLocator, elementZone, elementLocator, index, click, callback) {
			if (index === undefined) 
				index = 0;
			if (click === undefined) 
				click = true;
			execute("elementListPick", [listZone, listLocator, elementZone, elementLocator, index, click], null, callback);
		}

		function elementListSelect(listLocator, elementLocator, index, click, callback) {
			if (index === undefined) 
				index = 0;
			if (click === undefined) 
				click = true;
			execute("elementListSelect", [listLocator, elementLocator, index, click], null, callback);
		}

		function elementListVisible(listLocator, elementLocator, index, callback) {
			if (index === undefined) 
				index = 0;
			return execute("elementListVisible", [listLocator, elementLocator, index], "found", callback);
		}

		function elementScrollToTableRow2(tableLocator, tableIndex, rowIndex, callback) {
			if (tableIndex === undefined) 
				tableIndex = 0;
			if (rowIndex === undefined) 
				rowIndex = 0;
			execute("elementScrollToTableRow", [tableLocator, tableIndex, rowIndex], null, callback);
		}

		function elementScrollToTableRow(zone, tableLocator, tableIndex, rowIndex, callback) {
			if (tableIndex === undefined) 
				tableIndex = 0;
			if (rowIndex === undefined) 
				rowIndex = 0;
			execute("elementScrollToTableRow", [zone, tableLocator, tableIndex, rowIndex], null, callback);
		}

		function elementSendText(zone, element, index, text, callback) {
			if (index === undefined) 
				index = 0;
			execute("elementSendText", [zone, element, index, text], null, callback);
		}

		function elementSetProperty(zone, element, index, property, value, callback) {
			if (index === undefined) 
				index = 0;
			return execute("elementSetProperty", [zone, element, index, property, value], "text", callback);
		}

		function elementSwipe(zone, element, index, direction, offset, time, callback) {
			if (index === undefined) 
				index = 0;
			if (offset === undefined) 
				offset = 0;
			if (time === undefined) 
				time = 2000;
			execute("elementSwipe", [zone, element, index, direction, offset, time], null, callback);
		}

		function elementSwipeWhileNotFound(componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex, delay, rounds, click, callback) {
			if (offset === undefined) 
				offset = 0;
			if (swipeTime === undefined) 
				swipeTime = 2000;
			if (elementtofindindex === undefined) 
				elementtofindindex = 0;
			if (delay === undefined) 
				delay = 1000;
			if (rounds === undefined) 
				rounds = 5;
			if (click === undefined) 
				click = true;
			return execute("elementSwipeWhileNotFound", [componentZone, componentElement, direction, offset, swipeTime, elementfindzone, elementtofind, elementtofindindex, delay, rounds, click], "found", callback);
		}

		function endTransaction(name, callback) {
			execute("endTransaction", [name], null, callback);
		}

		function exit(callback) {
			execute("exit", [], null, callback);
		}

		function extractLanguageFiles(application, directoryPath, allowOverwrite, callback) {
			if (allowOverwrite === undefined) 
				allowOverwrite = true;
			execute("extractLanguageFiles", [application, directoryPath, allowOverwrite], null, callback);
		}

		function findElements(zone, parent, by, value, callback) {
			return execute("findElements", [zone, parent, by, value], "textArray", callback);
		}

		function flick(direction, offset, callback) {
			if (offset === undefined) 
				offset = 0;
			execute("flick", [direction, offset], null, callback);
		}

		function flickCoordinate(x, y, direction, callback) {
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("flickCoordinate", [x, y, direction], null, callback);
		}

		function flickElement(zone, element, index, direction, callback) {
			if (index === undefined) 
				index = 0;
			execute("flickElement", [zone, element, index, direction], null, callback);
		}

		function forceTouch(zone, element, index, duration, force, dragDistanceX, dragDistanceY, dragDuration, callback) {
			if (index === undefined) 
				index = 0;
			if (duration === undefined) 
				duration = 100;
			if (force === undefined) 
				force = 100;
			if (dragDistanceX === undefined) 
				dragDistanceX = 0;
			if (dragDistanceY === undefined) 
				dragDistanceY = 0;
			if (dragDuration === undefined) 
				dragDuration = 1500;
			execute("forceTouch", [zone, element, index, duration, force, dragDistanceX, dragDistanceY, dragDuration], null, callback);
		}

		function generateReport(callback) {
			return execute("generateReport", [], "text", callback);
		}

		function generateReport2(releaseClient, callback) {
			if (releaseClient === undefined) 
				releaseClient = true;
			return execute("generateReport", [releaseClient], "text", callback);
		}

		function generateReport(releaseClient, propFilePath, callback) {
			if (releaseClient === undefined) 
				releaseClient = true;
			return execute("generateReport", [releaseClient, propFilePath], "text", callback);
		}

		function getAllSms(timeout, callback) {
			if (timeout === undefined) 
				timeout = 5000;
			return execute("getAllSms", [timeout], "textArray", callback);
		}

		function getAllValues(zone, element, property, callback) {
			return execute("getAllValues", [zone, element, property], "textArray", callback);
		}

		function getAllZonesWithElement(element, callback) {
			return execute("getAllZonesWithElement", [element], "text", callback);
		}

		function getAvailableAgentPort(callback) {
			return execute("getAvailableAgentPort", [], "port", callback);
		}

		function getAvailableAgentPort2(featureName, callback) {
			return execute("getAvailableAgentPort", [featureName], "port", callback);
		}

		function getBrowserTabIdList(callback) {
			return execute("getBrowserTabIdList", [], "textArray", callback);
		}

		function getConnectedDevices(callback) {
			return execute("getConnectedDevices", [], "text", callback);
		}

		function getContextList(callback) {
			return execute("getContextList", [], "textArray", callback);
		}

		function getCoordinateColor(x, y, callback) {
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			return execute("getCoordinateColor", [x, y], "color", callback);
		}

		function getCounter(counterName, callback) {
			return execute("getCounter", [counterName], "text", callback);
		}

		function getCurrentApplicationName(callback) {
			return execute("getCurrentApplicationName", [], "text", callback);
		}

		function getCurrentBrowserTabId(callback) {
			return execute("getCurrentBrowserTabId", [], "text", callback);
		}

		function getDefaultTimeout(callback) {
			return execute("getDefaultTimeout", [], "timeout", callback);
		}

		function getDeviceLog(callback) {
			return execute("getDeviceLog", [], "path", callback);
		}

		function getDeviceProperty(key, callback) {
			return execute("getDeviceProperty", [key], "text", callback);
		}

		function getDevicesInformation(callback) {
			return execute("getDevicesInformation", [], "text", callback);
		}

		function getElementCount(zone, element, callback) {
			return execute("getElementCount", [zone, element], "count", callback);
		}

		function getElementCountIn(zoneName, elementSearch, index, direction, elementCountZone, elementCount, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			return execute("getElementCountIn", [zoneName, elementSearch, index, direction, elementCountZone, elementCount, width, height], "count", callback);
		}

		function getInstalledApplications(callback) {
			return execute("getInstalledApplications", [], "text", callback);
		}

		function getLastSMS(timeout, callback) {
			if (timeout === undefined) 
				timeout = 5000;
			return execute("getLastSMS", [timeout], "text", callback);
		}

		function getMonitorsData(cSVfilepath, callback) {
			return execute("getMonitorsData", [cSVfilepath], "text", callback);
		}

		function getNetworkConnection(connection, callback) {
			return execute("getNetworkConnection", [connection], "found", callback);
		}

		function getNVProfiles(callback) {
			return execute("getNVProfiles", [], "textArray", callback);
		}

		function getPickerValues(zone, pickerElement, index, wheelIndex, callback) {
			if (index === undefined) 
				index = 0;
			if (wheelIndex === undefined) 
				wheelIndex = 0;
			return execute("getPickerValues", [zone, pickerElement, index, wheelIndex], "textArray", callback);
		}

		function getPosition(zone, element, callback) {
			return execute("getPosition", [zone, element], "click", callback);
		}

		function getPositionWindowRelative(zone, element, callback) {
			return execute("getPositionWindowRelative", [zone, element], "centerWinRelative", callback);
		}

		function getProperty(property, callback) {
			return execute("getProperty", [property], "text", callback);
		}

		function getSimCard(callback) {
			return execute("getSimCard", [], "text", callback);
		}

		function getSimCards(readyToUse, callback) {
			if (readyToUse === undefined) 
				readyToUse = true;
			return execute("getSimCards", [readyToUse], "textArray", callback);
		}

		function getTableCellText(zone, headerElement, headerIndex, rowElement, rowIndex, width, height, callback) {
			if (headerIndex === undefined) 
				headerIndex = 0;
			if (rowIndex === undefined) 
				rowIndex = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			return execute("getTableCellText", [zone, headerElement, headerIndex, rowElement, rowIndex, width, height], "text", callback);
		}

		function getText(zone, callback) {
			return execute("getText", [zone], "text", callback);
		}

		function getTextIn(zone, element, index, direction, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			return execute("getTextIn", [zone, element, index, direction, width, height], "text", callback);
		}

		function getTextIn2(zone, element, index, textZone, direction, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			return execute("getTextIn", [zone, element, index, textZone, direction, width, height], "text", callback);
		}

		function getTextIn3(zone, element, index, textZone, direction, width, height, xOffset, yOffset, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			if (xOffset === undefined) 
				xOffset = 0;
			if (yOffset === undefined) 
				yOffset = 0;
			return execute("getTextIn", [zone, element, index, textZone, direction, width, height, xOffset, yOffset], "text", callback);
		}

		function getVisualDump(type, callback) {
			if (type === undefined) 
				type = Native;
			return execute("getVisualDump", [type], "text", callback);
		}

		function hybridClearCache(clearCookies, clearCache, callback) {
			if (clearCookies === undefined) 
				clearCookies = true;
			if (clearCache === undefined) 
				clearCache = true;
			execute("hybridClearCache", [clearCookies, clearCache], null, callback);
		}

		function hybridGetHtml(webViewLocator, index, callback) {
			if (index === undefined) 
				index = 0;
			return execute("hybridGetHtml", [webViewLocator, index], "text", callback);
		}

		function hybridRunJavascript(webViewLocator, index, script, callback) {
			if (index === undefined) 
				index = 0;
			return execute("hybridRunJavascript", [webViewLocator, index, script], "text", callback);
		}

		function hybridSelect(webViewLocator, index, method, value, select, callback) {
			if (index === undefined) 
				index = 0;
			execute("hybridSelect", [webViewLocator, index, method, value, select], null, callback);
		}

		function hybridWaitForPageLoad(timeout, callback) {
			if (timeout === undefined) 
				timeout = 10000;
			execute("hybridWaitForPageLoad", [timeout], null, callback);
		}

		function install(path, sign, callback) {
			if (sign === undefined) 
				sign = false;
			return execute("install", [path, sign], "found", callback);
		}

		function install2(path, instrument, keepData, callback) {
			if (instrument === undefined) 
				instrument = true;
			if (keepData === undefined) 
				keepData = false;
			return execute("install", [path, instrument, keepData], "found", callback);
		}

		function installWithCustomKeystore(path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword, callback) {
			if (instrument === undefined) 
				instrument = true;
			if (keepData === undefined) 
				keepData = false;
			return execute("installWithCustomKeystore", [path, instrument, keepData, keystorePath, keystorePassword, keyAlias, keyPassword], "found", callback);
		}

		function isElementBlank(zone, element, index, colorGroups, callback) {
			if (index === undefined) 
				index = 0;
			if (colorGroups === undefined) 
				colorGroups = 10;
			return execute("isElementBlank", [zone, element, index, colorGroups], "found", callback);
		}

		function isElementFound2(zone, element, callback) {
			return execute("isElementFound", [zone, element], "found", callback);
		}

		function isElementFound(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			return execute("isElementFound", [zone, element, index], "found", callback);
		}

		function isFoundIn(zone, searchElement, index, direction, elementFindZone, elementToFind, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			return execute("isFoundIn", [zone, searchElement, index, direction, elementFindZone, elementToFind, width, height], "found", callback);
		}

		function isTextFound(zone, element, ignoreCase, callback) {
			return execute("isTextFound", [zone, element, ignoreCase], "found", callback);
		}

		function launch2(applicationIdentifier, launchOptions, callback) {
			execute("launch", [applicationIdentifier, launchOptions], null, callback);
		}

		function launch(activityURL, instrument, stopIfRunning, callback) {
			if (instrument === undefined) 
				instrument = true;
			if (stopIfRunning === undefined) 
				stopIfRunning = false;
			execute("launch", [activityURL, instrument, stopIfRunning], null, callback);
		}

		function listSelect(sendRest, sendNavigation, delay, textToIdentify, color, rounds, sendonfind, callback) {
			if (sendRest === undefined) 
				sendRest = {UP};
			if (sendNavigation === undefined) 
				sendNavigation = {DOWN};
			if (delay === undefined) 
				delay = 500;
			if (rounds === undefined) 
				rounds = 5;
			if (sendonfind === undefined) 
				sendonfind = {ENTER};
			return execute("listSelect", [sendRest, sendNavigation, delay, textToIdentify, color, rounds, sendonfind], "found", callback);
		}

		function longClick(zone, element, index, clickCount, x, y, callback) {
			if (index === undefined) 
				index = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("longClick", [zone, element, index, clickCount, x, y], null, callback);
		}

		function maximize(callback) {
			execute("maximize", [], null, callback);
		}

		function multiClick(zone, element, index, fingerIndex, callback) {
			if (index === undefined) 
				index = 0;
			execute("multiClick", [zone, element, index, fingerIndex], null, callback);
		}

		function multiClickCoordinate(x, y, fingerIndex, callback) {
			execute("multiClickCoordinate", [x, y, fingerIndex], null, callback);
		}

		function multiClickOffset(zone, element, index, x, y, fingerIndex, callback) {
			if (index === undefined) 
				index = 0;
			execute("multiClickOffset", [zone, element, index, x, y, fingerIndex], null, callback);
		}

		function multiSwipe(direction, offset, time, fingerIndex, callback) {
			if (time === undefined) 
				time = 500;
			execute("multiSwipe", [direction, offset, time, fingerIndex], null, callback);
		}

		function multiTouchDown(zone, element, index, fingerIndex, callback) {
			if (index === undefined) 
				index = 0;
			execute("multiTouchDown", [zone, element, index, fingerIndex], null, callback);
		}

		function multiTouchDownCoordinate(x, y, fingerIndex, callback) {
			execute("multiTouchDownCoordinate", [x, y, fingerIndex], null, callback);
		}

		function multiTouchMove(zone, element, index, fingerIndex, callback) {
			if (index === undefined) 
				index = 0;
			execute("multiTouchMove", [zone, element, index, fingerIndex], null, callback);
		}

		function multiTouchMoveCoordinate(x, y, fingerIndex, callback) {
			execute("multiTouchMoveCoordinate", [x, y, fingerIndex], null, callback);
		}

		function multiTouchUp(fingerIndex, callback) {
			execute("multiTouchUp", [fingerIndex], null, callback);
		}

		function multiWait(time, fingerIndex, callback) {
			if (time === undefined) 
				time = 500;
			execute("multiWait", [time, fingerIndex], null, callback);
		}

		function openDevice(callback) {
			execute("openDevice", [], null, callback);
		}

		function p2cx(percentage, callback) {
			if (percentage === undefined) 
				percentage = 0;
			return execute("p2cx", [percentage], "pixel", callback);
		}

		function p2cy(percentage, callback) {
			if (percentage === undefined) 
				percentage = 0;
			return execute("p2cy", [percentage], "pixel", callback);
		}

		function performMultiGesture(callback) {
			execute("performMultiGesture", [], null, callback);
		}

		function pinch(inside, x, y, radius, callback) {
			if (inside === undefined) 
				inside = true;
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			if (radius === undefined) 
				radius = 100;
			return execute("pinch", [inside, x, y, radius], "found", callback);
		}

		function pinch2(inside, x, y, radius, horizontal, callback) {
			if (inside === undefined) 
				inside = true;
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			if (radius === undefined) 
				radius = 100;
			if (horizontal === undefined) 
				horizontal = false;
			return execute("pinch", [inside, x, y, radius, horizontal], "found", callback);
		}

		function portForward(localPort, remotePort, callback) {
			return execute("PortForward", [localPort, remotePort], "port", callback);
		}

		function pressWhileNotFound(zone, elementtoclick, elementtofind, timeout, delay, callback) {
			if (timeout === undefined) 
				timeout = 10000;
			if (delay === undefined) 
				delay = 0;
			execute("pressWhileNotFound", [zone, elementtoclick, elementtofind, timeout, delay], null, callback);
		}

		function pressWhileNotFound2(zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex, timeout, delay, callback) {
			if (elementtoclickindex === undefined) 
				elementtoclickindex = 0;
			if (elementtofindindex === undefined) 
				elementtofindindex = 0;
			if (timeout === undefined) 
				timeout = 10000;
			if (delay === undefined) 
				delay = 0;
			execute("pressWhileNotFound", [zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex, timeout, delay], null, callback);
		}

		function reboot(timeout, callback) {
			if (timeout === undefined) 
				timeout = 120000;
			return execute("reboot", [timeout], "status", callback);
		}

		function receiveIncomingCall(fromNumber, hangupInSeconds, callback) {
			execute("receiveIncomingCall", [fromNumber, hangupInSeconds], null, callback);
		}

		function receiveIncomingSMS(fromNumber, msg, callback) {
			execute("receiveIncomingSMS", [fromNumber, msg], null, callback);
		}

		function releaseClient(callback) {
			execute("releaseClient", [], null, callback);
		}

		function releaseDevice(deviceName, releaseAgent, removeFromDeviceList, releaseFromCloud, callback) {
			if (releaseAgent === undefined) 
				releaseAgent = true;
			if (removeFromDeviceList === undefined) 
				removeFromDeviceList = false;
			if (releaseFromCloud === undefined) 
				releaseFromCloud = true;
			execute("releaseDevice", [deviceName, releaseAgent, removeFromDeviceList, releaseFromCloud], null, callback);
		}

		function report(message, status, callback) {
			execute("report", [message, status], null, callback);
		}

		function reportWithImage(pathToImage, message, status, callback) {
			execute("report", [pathToImage, message, status], null, callback);
		}

		function resetDeviceBridge(callback) {
			execute("resetDeviceBridge", [], null, callback);
		}

		function resetDeviceBridgeOS(deviceType, callback) {
			execute("resetDeviceBridge", [deviceType], null, callback);
		}

		function rightClick(zone, element, index, clickCount, x, y, callback) {
			if (index === undefined) 
				index = 0;
			if (clickCount === undefined) 
				clickCount = 1;
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("rightClick", [zone, element, index, clickCount, x, y], null, callback);
		}

		function run(command, callback) {
			return execute("run", [command], "text", callback);
		}

		function LayoutTest(xml, callback) {
			return execute("runLayoutTest", [xml], "text", callback);
		}

		function runNativeAPICall(zone, element, index, script, callback) {
			if (index === undefined) 
				index = 0;
			return execute("runNativeAPICall", [zone, element, index, script], "text", callback);
		}

		function sendText(text, callback) {
			execute("sendText", [text], null, callback);
		}

		function sendWhileNotFound(toSend, zone, elementtofind, timeout, delay, callback) {
			if (timeout === undefined) 
				timeout = 10000;
			if (delay === undefined) 
				delay = 1000;
			execute("sendWhileNotFound", [toSend, zone, elementtofind, timeout, delay], null, callback);
		}

		function sendWhileNotFound2(toSend, zone, elementtofind, elementtofindindex, timeout, delay, callback) {
			if (elementtofindindex === undefined) 
				elementtofindindex = 0;
			if (timeout === undefined) 
				timeout = 10000;
			if (delay === undefined) 
				delay = 1000;
			execute("sendWhileNotFound", [toSend, zone, elementtofind, elementtofindindex, timeout, delay], null, callback);
		}

		function setApplicationTitle(title, callback) {
			execute("setApplicationTitle", [title], null, callback);
		}

		function setAuthenticationReply(reply, delay, callback) {
			if (reply === undefined) 
				reply = Success;
			if (delay === undefined) 
				delay = 0;
			execute("setAuthenticationReply", [reply, delay], null, callback);
		}

		function setContext(context, callback) {
			if (context === undefined) 
				context = NATIVE_APP;
			execute("setContext", [context], null, callback);
		}

		function setDefaultClickDownTime(downTime, callback) {
			if (downTime === undefined) 
				downTime = 100;
			execute("setDefaultClickDownTime", [downTime], null, callback);
		}

		function setDefaultTimeout(newTimeout, callback) {
			if (newTimeout === undefined) 
				newTimeout = 20000;
			return execute("setDefaultTimeout", [newTimeout], "timeout", callback);
		}

		function setDefaultWebView(webViewLocator, callback) {
			execute("setDefaultWebView", [webViewLocator], null, callback);
		}

		function setDevice(device, callback) {
			execute("setDevice", [device], null, callback);
		}

		function setDragStartDelay(delay, callback) {
			if (delay === undefined) 
				delay = 0;
			execute("setDragStartDelay", [delay], null, callback);
		}

		function setInKeyDelay(delay, callback) {
			if (delay === undefined) 
				delay = 50;
			execute("setInKeyDelay", [delay], null, callback);
		}

		function setKeyToKeyDelay(delay, callback) {
			if (delay === undefined) 
				delay = 50;
			execute("setKeyToKeyDelay", [delay], null, callback);
		}

		function setLanguage(language, callback) {
			execute("setLanguage", [language], null, callback);
		}

		function setLanguagePropertiesFile(propertiesfile, callback) {
			execute("setLanguagePropertiesFile", [propertiesfile], null, callback);
		}

		function setLocation(latitude, longitude, callback) {
			if (latitude === undefined) 
				latitude = 0.0;
			if (longitude === undefined) 
				longitude = 0.0;
			execute("setLocation", [latitude, longitude], null, callback);
		}

		function setMonitorPollingInterval(timemilli, callback) {
			if (timemilli === undefined) 
				timemilli = 30000;
			execute("setMonitorPollingInterval", [timemilli], null, callback);
		}

		function setMonitorTestState(testStatus, callback) {
			execute("setMonitorTestState", [testStatus], null, callback);
		}

		function setNetworkConditions(profile, callback) {
			execute("setNetworkConditions", [profile], null, callback);
		}

		function setNetworkConditions2(profile, duration, callback) {
			if (duration === undefined) 
				duration = 0;
			execute("setNetworkConditions", [profile, duration], null, callback);
		}

		function setNetworkConnection(connection, enable, callback) {
			execute("setNetworkConnection", [connection, enable], null, callback);
		}

		function setOcrIgnoreCase(ignoreCase, callback) {
			execute("setOcrIgnoreCase", [ignoreCase], null, callback);
		}

		function setOcrTrainingFilePath(trainingPath, callback) {
			execute("setOcrTrainingFilePath", [trainingPath], null, callback);
		}

		function setPickerValues(zoneName, elementName, index, wheelIndex, value, callback) {
			if (index === undefined) 
				index = 0;
			return execute("setPickerValues", [zoneName, elementName, index, wheelIndex, value], "text", callback);
		}

		function setProjectBaseDirectory(projectBaseDirectory, callback) {
			execute("setProjectBaseDirectory", [projectBaseDirectory], null, callback);
		}

		function setProperty(key, value, callback) {
			execute("setProperty", [key, value], null, callback);
		}

		function setRedToBlue(redToBlue, callback) {
			execute("setRedToBlue", [redToBlue], null, callback);
		}

		function setReporter(reporterName, directory, callback) {
			if (reporterName === undefined) 
				reporterName = html;
			return execute("setReporter", [reporterName, directory], "text", callback);
		}

		function setReporter2(reporterName, directory, testName, callback) {
			if (reporterName === undefined) 
				reporterName = html;
			return execute("setReporter", [reporterName, directory, testName], "text", callback);
		}

		function setShowImageAsLink(showImageAsLink, callback) {
			execute("setShowImageAsLink", [showImageAsLink], null, callback);
		}

		function setShowImageInReport(showImageInReport, callback) {
			if (showImageInReport === undefined) 
				showImageInReport = true;
			execute("setShowImageInReport", [showImageInReport], null, callback);
		}

		function setShowPassImageInReport(showPassImageInReport, callback) {
			if (showPassImageInReport === undefined) 
				showPassImageInReport = true;
			execute("setShowPassImageInReport", [showPassImageInReport], null, callback);
		}

		function setShowReport(showReport, callback) {
			if (showReport === undefined) 
				showReport = true;
			execute("setShowReport", [showReport], null, callback);
		}

		function setSimCard(simCardName, callback) {
			execute("setSimCard", [simCardName], null, callback);
		}

		function setSpeed(speed, callback) {
			execute("setSpeed", [speed], null, callback);
		}

		function setTestStatus(status, message, callback) {
			if (status === undefined) 
				status = true;
			execute("setTestStatus", [status, message], null, callback);
		}

		function setWebAutoScroll(autoScroll, callback) {
			if (autoScroll === undefined) 
				autoScroll = true;
			execute("setWebAutoScroll", [autoScroll], null, callback);
		}

		function setWindowSize(width, height, callback) {
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			execute("setWindowSize", [width, height], null, callback);
		}

		function shake(callback) {
			execute("shake", [], null, callback);
		}

		function simulateCapture(picturePath, callback) {
			execute("simulateCapture", [picturePath], null, callback);
		}

		function sleep(time, callback) {
			if (time === undefined) 
				time = 1000;
			execute("sleep", [time], null, callback);
		}

		function startAudioPlay(audioFile, callback) {
			execute("startAudioPlay", [audioFile], null, callback);
		}

		function startAudioRecording(audioFile, callback) {
			execute("startAudioRecording", [audioFile], null, callback);
		}

		function startCall(skypeUser, skypePassword, number, duration, callback) {
			if (duration === undefined) 
				duration = 0;
			execute("startCall", [skypeUser, skypePassword, number, duration], null, callback);
		}

		function startLoggingDevice(path, callback) {
			execute("startLoggingDevice", [path], null, callback);
		}

		function startMonitor(packageName, callback) {
			execute("startMonitor", [packageName], null, callback);
		}

		function startMultiGesture(name, callback) {
			execute("startMultiGesture", [name], null, callback);
		}

		function startStepsGroup(caption, callback) {
			execute("startStepsGroup", [caption], null, callback);
		}

		function startTransaction(name, callback) {
			execute("startTransaction", [name], null, callback);
		}

		function startVideoRecord(callback) {
			execute("startVideoRecord", [], null, callback);
		}

		function stopAudioPlay(callback) {
			execute("stopAudioPlay", [], null, callback);
		}

		function stopAudioRecording(callback) {
			execute("stopAudioRecording", [], null, callback);
		}

		function stopLoggingDevice(callback) {
			return execute("stopLoggingDevice", [], "text", callback);
		}

		function stopStepsGroup(callback) {
			execute("stopStepsGroup", [], null, callback);
		}

		function stopVideoRecord(callback) {
			return execute("stopVideoRecord", [], "text", callback);
		}

		function swipe(direction, offset, callback) {
			if (offset === undefined) 
				offset = 0;
			execute("swipe", [direction, offset], null, callback);
		}

		function swipe2(direction, offset, time, callback) {
			if (offset === undefined) 
				offset = 0;
			if (time === undefined) 
				time = 500;
			execute("swipe", [direction, offset, time], null, callback);
		}

		function swipeWhileNotFound(direction, offset, zone, elementtofind, delay, rounds, click, callback) {
			if (offset === undefined) 
				offset = 0;
			if (delay === undefined) 
				delay = 1000;
			if (rounds === undefined) 
				rounds = 5;
			if (click === undefined) 
				click = true;
			return execute("swipeWhileNotFound", [direction, offset, zone, elementtofind, delay, rounds, click], "found", callback);
		}

		function swipeWhileNotFound3(direction, offset, swipeTime, zone, elementtofind, delay, rounds, click, callback) {
			if (offset === undefined) 
				offset = 0;
			if (swipeTime === undefined) 
				swipeTime = 2000;
			if (delay === undefined) 
				delay = 1000;
			if (rounds === undefined) 
				rounds = 5;
			if (click === undefined) 
				click = true;
			return execute("swipeWhileNotFound", [direction, offset, swipeTime, zone, elementtofind, delay, rounds, click], "found", callback);
		}

		function swipeWhileNotFound2(direction, offset, swipeTime, zone, elementtofind, elementtofindindex, delay, rounds, click, callback) {
			if (offset === undefined) 
				offset = 0;
			if (swipeTime === undefined) 
				swipeTime = 2000;
			if (elementtofindindex === undefined) 
				elementtofindindex = 0;
			if (delay === undefined) 
				delay = 1000;
			if (rounds === undefined) 
				rounds = 5;
			if (click === undefined) 
				click = true;
			return execute("swipeWhileNotFound", [direction, offset, swipeTime, zone, elementtofind, elementtofindindex, delay, rounds, click], "found", callback);
		}

		function switchToBrowserTab(tabId, callback) {
			execute("switchToBrowserTab", [tabId], null, callback);
		}

		function sync(silentTime, sensitivity, timeout, callback) {
			if (silentTime === undefined) 
				silentTime = 2000;
			if (sensitivity === undefined) 
				sensitivity = 0;
			if (timeout === undefined) 
				timeout = 10000;
			return execute("sync", [silentTime, sensitivity, timeout], "found", callback);
		}

		function syncElements(silentTime, timeout, callback) {
			if (silentTime === undefined) 
				silentTime = 2000;
			if (timeout === undefined) 
				timeout = 10000;
			return execute("syncElements", [silentTime, timeout], "found", callback);
		}

		function textFilter(color, sensitivity, callback) {
			if (sensitivity === undefined) 
				sensitivity = 15;
			execute("textFilter", [color, sensitivity], null, callback);
		}

		function touchDown(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			execute("touchDown", [zone, element, index], null, callback);
		}

		function touchDownCoordinate(x, y, callback) {
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("touchDownCoordinate", [x, y], null, callback);
		}

		function touchMove(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			execute("touchMove", [zone, element, index], null, callback);
		}

		function touchMoveCoordinate(x, y, callback) {
			if (x === undefined) 
				x = 0;
			if (y === undefined) 
				y = 0;
			execute("touchMoveCoordinate", [x, y], null, callback);
		}

		function touchUp(callback) {
			execute("touchUp", [], null, callback);
		}

		function uninstall(application, callback) {
			return execute("uninstall", [application], "found", callback);
		}

		function verifyElementFound(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			execute("verifyElementFound", [zone, element, index], null, callback);
		}

		function verifyElementNotFound(zone, element, index, callback) {
			if (index === undefined) 
				index = 0;
			execute("verifyElementNotFound", [zone, element, index], null, callback);
		}

		function verifyIn(zone, searchElement, index, direction, elementFindZone, elementToFind, width, height, callback) {
			if (index === undefined) 
				index = 0;
			if (width === undefined) 
				width = 0;
			if (height === undefined) 
				height = 0;
			execute("verifyIn", [zone, searchElement, index, direction, elementFindZone, elementToFind, width, height], null, callback);
		}

		function waitForAudioPlayEnd(timeout, callback) {
			execute("waitForAudioPlayEnd", [timeout], null, callback);
		}

		function waitForElement(zone, element, index, timeout, callback) {
			if (index === undefined) 
				index = 0;
			if (timeout === undefined) 
				timeout = 10000;
			return execute("waitForElement", [zone, element, index, timeout], "found", callback);
		}

		function waitForElementToVanish(zone, element, index, timeout, callback) {
			if (index === undefined) 
				index = 0;
			if (timeout === undefined) 
				timeout = 10000;
			return execute("waitForElementToVanish", [zone, element, index, timeout], "found", callback);
		}

		function waitForWindow(name, timeout, callback) {
			if (timeout === undefined) 
				timeout = 10000;
			return execute("waitForWindow", [name, timeout], "found", callback);
		}



	return {
		initialize: initialize,
		execute: execute,
		getLastCommandResultMap: getLastCommandResultMap,
		pingServer: pingServer,
		waitForDevice: waitForDevice,
		activateVoiceAssistance: activateVoiceAssistance,
		addDevice: addDevice,
		applicationClearData: applicationClearData,
		applicationClose: applicationClose,
		capture: capture,
		captureLine: captureLine,
		captureElement: captureElement,
		clearAllSms: clearAllSms,
		clearDeviceLog: clearDeviceLog,
		clearLocation: clearLocation,
		click: click,
		clickOffset: clickOffset,
		clickCoordinate: clickCoordinate,
		clickIn: clickIn,
		clickIn2: clickIn2,
		clickIn2_5: clickIn2_5,
		clickIn3: clickIn3,
		clickTableCell: clickTableCell,
		closeDevice: closeDevice,
		closeDeviceReflection: closeDeviceReflection,
		closeKeyboard: closeKeyboard,
		collectSupportData: collectSupportData,
		collectSupportData2: collectSupportData2,
		deviceAction: deviceAction,
		drag: drag,
		dragCoordinates: dragCoordinates,
		dragCoordinates2: dragCoordinates2,
		dragDrop: dragDrop,
		drop: drop,
		elementGetProperty: elementGetProperty,
		elementGetTableRowsCount2: elementGetTableRowsCount2,
		elementGetTableRowsCount: elementGetTableRowsCount,
		elementGetTableValue: elementGetTableValue,
		elementGetText: elementGetText,
		elementListPick: elementListPick,
		elementListSelect: elementListSelect,
		elementListVisible: elementListVisible,
		elementScrollToTableRow2: elementScrollToTableRow2,
		elementScrollToTableRow: elementScrollToTableRow,
		elementSendText: elementSendText,
		elementSetProperty: elementSetProperty,
		elementSwipe: elementSwipe,
		elementSwipeWhileNotFound: elementSwipeWhileNotFound,
		endTransaction: endTransaction,
		exit: exit,
		extractLanguageFiles: extractLanguageFiles,
		findElements: findElements,
		flick: flick,
		flickCoordinate: flickCoordinate,
		flickElement: flickElement,
		forceTouch: forceTouch,
		generateReport: generateReport,
		generateReport2: generateReport2,
		generateReport: generateReport,
		getAllSms: getAllSms,
		getAllValues: getAllValues,
		getAllZonesWithElement: getAllZonesWithElement,
		getAvailableAgentPort: getAvailableAgentPort,
		getAvailableAgentPort2: getAvailableAgentPort2,
		getBrowserTabIdList: getBrowserTabIdList,
		getConnectedDevices: getConnectedDevices,
		getContextList: getContextList,
		getCoordinateColor: getCoordinateColor,
		getCounter: getCounter,
		getCurrentApplicationName: getCurrentApplicationName,
		getCurrentBrowserTabId: getCurrentBrowserTabId,
		getDefaultTimeout: getDefaultTimeout,
		getDeviceLog: getDeviceLog,
		getDeviceProperty: getDeviceProperty,
		getDevicesInformation: getDevicesInformation,
		getElementCount: getElementCount,
		getElementCountIn: getElementCountIn,
		getInstalledApplications: getInstalledApplications,
		getLastSMS: getLastSMS,
		getMonitorsData: getMonitorsData,
		getNetworkConnection: getNetworkConnection,
		getNVProfiles: getNVProfiles,
		getPickerValues: getPickerValues,
		getPosition: getPosition,
		getPositionWindowRelative: getPositionWindowRelative,
		getProperty: getProperty,
		getSimCard: getSimCard,
		getSimCards: getSimCards,
		getTableCellText: getTableCellText,
		getText: getText,
		getTextIn: getTextIn,
		getTextIn2: getTextIn2,
		getTextIn3: getTextIn3,
		getVisualDump: getVisualDump,
		hybridClearCache: hybridClearCache,
		hybridGetHtml: hybridGetHtml,
		hybridRunJavascript: hybridRunJavascript,
		hybridSelect: hybridSelect,
		hybridWaitForPageLoad: hybridWaitForPageLoad,
		install: install,
		install2: install2,
		installWithCustomKeystore: installWithCustomKeystore,
		isElementBlank: isElementBlank,
		isElementFound2: isElementFound2,
		isElementFound: isElementFound,
		isFoundIn: isFoundIn,
		isTextFound: isTextFound,
		launch2: launch2,
		launch: launch,
		listSelect: listSelect,
		longClick: longClick,
		maximize: maximize,
		multiClick: multiClick,
		multiClickCoordinate: multiClickCoordinate,
		multiClickOffset: multiClickOffset,
		multiSwipe: multiSwipe,
		multiTouchDown: multiTouchDown,
		multiTouchDownCoordinate: multiTouchDownCoordinate,
		multiTouchMove: multiTouchMove,
		multiTouchMoveCoordinate: multiTouchMoveCoordinate,
		multiTouchUp: multiTouchUp,
		multiWait: multiWait,
		openDevice: openDevice,
		p2cx: p2cx,
		p2cy: p2cy,
		performMultiGesture: performMultiGesture,
		pinch: pinch,
		pinch2: pinch2,
		portForward: portForward,
		pressWhileNotFound: pressWhileNotFound,
		pressWhileNotFound2: pressWhileNotFound2,
		reboot: reboot,
		receiveIncomingCall: receiveIncomingCall,
		receiveIncomingSMS: receiveIncomingSMS,
		releaseClient: releaseClient,
		releaseDevice: releaseDevice,
		report: report,
		reportWithImage: reportWithImage,
		resetDeviceBridge: resetDeviceBridge,
		resetDeviceBridgeOS: resetDeviceBridgeOS,
		rightClick: rightClick,
		run: run,
		LayoutTest: LayoutTest,
		runNativeAPICall: runNativeAPICall,
		sendText: sendText,
		sendWhileNotFound: sendWhileNotFound,
		sendWhileNotFound2: sendWhileNotFound2,
		setApplicationTitle: setApplicationTitle,
		setAuthenticationReply: setAuthenticationReply,
		setContext: setContext,
		setDefaultClickDownTime: setDefaultClickDownTime,
		setDefaultTimeout: setDefaultTimeout,
		setDefaultWebView: setDefaultWebView,
		setDevice: setDevice,
		setDragStartDelay: setDragStartDelay,
		setInKeyDelay: setInKeyDelay,
		setKeyToKeyDelay: setKeyToKeyDelay,
		setLanguage: setLanguage,
		setLanguagePropertiesFile: setLanguagePropertiesFile,
		setLocation: setLocation,
		setMonitorPollingInterval: setMonitorPollingInterval,
		setMonitorTestState: setMonitorTestState,
		setNetworkConditions: setNetworkConditions,
		setNetworkConditions2: setNetworkConditions2,
		setNetworkConnection: setNetworkConnection,
		setOcrIgnoreCase: setOcrIgnoreCase,
		setOcrTrainingFilePath: setOcrTrainingFilePath,
		setPickerValues: setPickerValues,
		setProjectBaseDirectory: setProjectBaseDirectory,
		setProperty: setProperty,
		setRedToBlue: setRedToBlue,
		setReporter: setReporter,
		setReporter2: setReporter2,
		setShowImageAsLink: setShowImageAsLink,
		setShowImageInReport: setShowImageInReport,
		setShowPassImageInReport: setShowPassImageInReport,
		setShowReport: setShowReport,
		setSimCard: setSimCard,
		setSpeed: setSpeed,
		setTestStatus: setTestStatus,
		setWebAutoScroll: setWebAutoScroll,
		setWindowSize: setWindowSize,
		shake: shake,
		simulateCapture: simulateCapture,
		sleep: sleep,
		startAudioPlay: startAudioPlay,
		startAudioRecording: startAudioRecording,
		startCall: startCall,
		startLoggingDevice: startLoggingDevice,
		startMonitor: startMonitor,
		startMultiGesture: startMultiGesture,
		startStepsGroup: startStepsGroup,
		startTransaction: startTransaction,
		startVideoRecord: startVideoRecord,
		stopAudioPlay: stopAudioPlay,
		stopAudioRecording: stopAudioRecording,
		stopLoggingDevice: stopLoggingDevice,
		stopStepsGroup: stopStepsGroup,
		stopVideoRecord: stopVideoRecord,
		swipe: swipe,
		swipe2: swipe2,
		swipeWhileNotFound: swipeWhileNotFound,
		swipeWhileNotFound3: swipeWhileNotFound3,
		swipeWhileNotFound2: swipeWhileNotFound2,
		switchToBrowserTab: switchToBrowserTab,
		sync: sync,
		syncElements: syncElements,
		textFilter: textFilter,
		touchDown: touchDown,
		touchDownCoordinate: touchDownCoordinate,
		touchMove: touchMove,
		touchMoveCoordinate: touchMoveCoordinate,
		touchUp: touchUp,
		uninstall: uninstall,
		verifyElementFound: verifyElementFound,
		verifyElementNotFound: verifyElementNotFound,
		verifyIn: verifyIn,
		waitForAudioPlayEnd: waitForAudioPlayEnd,
		waitForElement: waitForElement,
		waitForElementToVanish: waitForElementToVanish,
		waitForWindow: waitForWindow,
		getVersion: getVersion,
		setDebug: setDebug
	};




}();



module.exports = MobileClient;
