#!/usr/bin/ruby -w

require 'uri'
require 'base64'
require 'json'
require 'rest-client'



module Mobile
  require './client'

  class GridClient

    REST_API_PREFIX = '/api/v1/grid'
    REST_API_LOCK_DEVICE = '/wait-for-device'
    REST_API_GET_DEVICES = '/get-devices-information'
    REST_API_RELEASE_DEVICE = '/release-device'

    def self.new_client(userName, password, projectName, domain, port, isSecured)
      gridClient = allocate
      gridClient.init_mutex
      gridClient.init_credentials(userName, password, projectName)
      gridClient.init_address(domain, port, isSecured)
      gridClient
    end

    def self.new_client_access_key(accessKey, domain, port, isSecured)
      gridClient = allocate
      gridClient.init_mutex
      gridClient.init_credentials_access_key(accessKey)
      gridClient.init_address(domain, port, isSecured)
      gridClient
    end

    def self.new_client_url(userName, password, projectName, url)
      gridClient = allocate
      gridClient.init_mutex
      gridClient.init_credentials(userName, password, projectName)
      gridClient.init_address_url(url)
      gridClient
    end

    def self.new_client_url_access_key(accessKey, url)
      gridClient = allocate
      gridClient.init_mutex
      gridClient.init_credentials_access_key(accessKey)
      gridClient.init_address_url(url)
      gridClient
    end

    def init_address(domain, port, isSecured)
      # Address
      @domain = domain
      @port = port
      @isSecured = isSecured

      @url = buildURL(domain, port, isSecured)

    end

    def init_address_url(url)
      # Address
      @url = url
    end

    def init_credentials(userName, password, projectName)
      # Credentials
      @userName = userName
      @password = password
      @projectName = projectName
    end

    def init_credentials_access_key(accessKey)
      # Credentials
      @accessKey = accessKey
    end

    def init_mutex()
      @mutex_reserve = Mutex.new
      @mutex_release = Mutex.new
    end

    private def buildURL(domain, port, isSecured)
      s = StringIO.new

      s << (isSecured ? 'https://' : 'http://')
      s << domain
      s << ':'
      s << port
      s.string
    end

    private def getFullURL(request)
      s = StringIO.new

      s << @url
      s << REST_API_PREFIX
      s << request
      s.string
    end

    def getRequest(request)

      begin
        # Get ALL incidents
        response = RestClient.get(getFullURL(request),
                                  {:authorization => "Basic #{Base64.strict_encode64(@userName+':'+@password)}",
                                   :accept => 'application/json'})
        #puts "#{response.to_str}"

        response.to_str

      rescue => e
        ExperiLogger.instance.error(e)
      end

    end

    def postRequest(request, params)

      begin
        # Get ALL incidents
        response = RestClient.post(getFullURL(request),
                                   params,
                                   {:authorization => "Basic #{Base64.strict_encode64(@userName+':'+@password)}",
                                    :accept => 'application/json'})
        #  puts "#{response.to_str}"

        response.to_str

      rescue => e
        ExperiLogger.instance.error(e)
      end

    end

    def lockDeviceForExecution(testName, deviceQuery, reservationTimeInMinutes, timeout)

      @mutex_reserve.synchronize do

        startTime = Time.new

        sessionId = nil

        params = {  :"testName" => testName,
                    :"deviceQuery" => deviceQuery,
                    :"reservationTimeInMinutes" => reservationTimeInMinutes,
                    :"gridClientVersion" => getVersion,
                    :"releaseDevice" => true
        }

        while timeout > Time.new - startTime

          if (!sessionId.nil? && sessionId != '')
            params[:"sessionId"] = sessionId
          end


          json = postRequest(REST_API_LOCK_DEVICE, params)

          result = JSON.parse(json.to_str)

          if (result.include? 'status')
            status = result["status"]
          end

          if (result.include? 'message')
            message = result["message"]
          end

          case status
            when "SUCCESS"
              if (result.include? 'data')
                data = result["data"]

                if (data.include? 'managerUrl')
                  managerUrl = data["managerUrl"]
                end

                sessionId = data["sessionId"]

                # Get Internal Address
                ehmHostOrIp = data["ehmHostOrIp"]
                ehmPort = data["ehmPort"]
                # Get External Address
                ehmExternalHostOrIp = data["ehmExternalHostOrIp"]
                ehmExternalPort = data["ehmExternalPort"]

                ehmSecured = data["ehmSecured"].to_s == 'true'


                urlPathPrefix = nil
                # urlPrefixObj = data("ehmUrlPrefix")
                if (data.include? 'ehmUrlPrefix')
                  urlPathPrefix = data["ehmUrlPrefix"]
                end
                #
                if(!urlPathPrefix.nil? and urlPathPrefix != '')
                  # if urlPrefix is not empty, it means we are in single port more
                  # i.e. we will use the same host and port we used to connect to server and only
                  # url prefix will differ
                  # this is under assumption that the connection to cloud server was done also via
                  # single port
                  ehmHostOrIp = @domain
                  ehmPort = @port
                  ehmSecured = @isSecured

                elsif (ehmExternalHostOrIp != ehmHostOrIp || ehmExternalPort != ehmPort)

                  if(!ehmExternalHostOrIp.nil? and ehmExternalHostOrIp != '' and
                      !ehmExternalPort.nil? and ehmExternalPort != '')

                    pingExternalClient = Mobile::PingRubyClient.new(ehmExternalHostOrIp, ehmExternalPort, ehmSecured, sessionId, urlPathPrefix)
                    begin
                      pingExternalClient.pingServer()

                      ehmHostOrIp = ehmExternalHostOrIp
                      ehmPort = ehmExternalPort

                    rescue Exception => e
                      ExperiLogger.instance.info('Failed to connect to ExternalHost, try to use Internal address')
                    end
                  end
                end

                client = Mobile::ExecutionClient.new(self, ehmHostOrIp, ehmPort, ehmSecured, sessionId, urlPathPrefix)


                # link to view - support single port

                scheme = ehmSecured ? 'https://' : 'http://'
                link = 'Link for view: ' + scheme + ehmHostOrIp + ':' + ehmPort.to_s + urlPathPrefix
                message = message.sub('Link for view: ', link)


                ExperiLogger.instance.info(message)
                ExperiLogger.instance.info('The method lockDeviceForExecution creating client, HostOrIp: ' + ehmHostOrIp + ', Port: ' + ehmPort.to_s + ', secure: ' + ehmSecured.to_s)
                # puts 'Client version: ' + getVersionWithBuild()

                currentExecutionTime = Time.new - startTime

                timeToLockDevice = [currentExecutionTime, 20000].max
                ExperiLogger.instance.info('The method lockPreReservedDevice run with timeout ' + timeToLockDevice.to_s)

                begin
                  client.lockPreReservedDevice(timeToLockDevice);
                rescue Exception => e
                  releaseDeviceFromExecution(client)
                  ExperiLogger.instance.error('The method lockPreReservedDevice failed, Grid Node was released')
                  raise e, 'The method lockPreReservedDevice failed, Grid Node was released.'
                end

                ExperiLogger.instance.info('The method lockDeviceForExecution success with sessionId ' + sessionId)
                return client
              end
            when "QUEUED"
              if (result.include? 'data')
                data = result["data"]
                if (sessionId.nil?)
                  ExperiLogger.instance.info('The method lockDeviceForExecution queued with sessionId ' + data["sessionId"] + ' \n and message: ' + message)
                end

                sessionId = data["sessionId"]

              end

              sleep(2.5)
            else
              ExperiLogger.instance.error('The lockDeviceForExecution failed with error: ' + message)
              raise 'The lockDeviceForExecution failed with error: ' + message
          end

        end
      end
    end

    def getDevicesInformation
      json = getRequest(REST_API_GET_DEVICES)

      result = JSON.parse(json.to_str)

      if (result.include? 'data')
        result["data"]
      else
        ExperiLogger.instance.error('getDevicesInformation command return wrong response.')
      end
    end

    def releaseDeviceFromExecution(executorClient)

      @mutex_release.synchronize do
        sessionId = executorClient.getSessionId()

        if (sessionId.nil? or sessionId == '')
          ExperiLogger.instance.error('The method releaseDeviceFromExecution failed, client is invalid.')
          raise 'The method releaseDeviceFromExecution failed, client is invalid.'
        end

        params = { :'sessionId' => sessionId }

        json = postRequest(REST_API_RELEASE_DEVICE, params)

        result = JSON.parse(json.to_str)

        if (result.include? 'data')
          result["data"]
        else
          ExperiLogger.instance.error('releaseDeviceFromExecution command return wrong response.')
        end
      end
    end

    def getVersion()
      Mobile::Client::VERSION
    end

  end # Class

end # Module MOBILE