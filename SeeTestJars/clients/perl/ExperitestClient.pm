package SeeTest;

$RPC::XML::ENCODING = "utf-8";
use strict;
use vars qw(@ISA);
use RPC::XML;
use Data::Dumper;
use LWP::UserAgent;
#@ISA = qw/RPC::XML::Client/;
use base "RPC::XML::Client";
use Digest::MD5;
our $hashmap = {};

sub new {
    my $this = shift;
    my ($host, $port, $useSessionID) = (shift, shift, shift);
    if (not defined ($host)) {
        $host = "127.0.0.1";
    }
    if (not defined ($port)) {
        $port = "8888";
    }
    if (not defined ($useSessionID)) {
        $useSessionID = 0;
    }
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self = $self->SUPER::new("http://$host:$port/xmlrpc");
    $self->{DIEONFAIL}   = 1;
    $self->{HOST} = $host;
    $self->{MOBILE_LISTENERS} = {};
    if ($useSessionID){
	    $self->{ClientID} = "clientID:Perl:version=" . getVersion() . ":" . Digest::MD5::md5_base64( rand );
	} else {
		$self->{ClientID} = undef;
	}
    #my $tst = $self->send_request('agent.setClient', ($self->{ClientID}));
    return $self;
}

sub isDieOnFail() {
    my $self = shift;
    return $self->{DIEONFAIL};
}

sub execute () {
		
    my ($self, $methodName, @args) = @_;
    
    if (scalar(@args) > 12) {
        die "Unsupported number of arguments";
    } else {
    	if (defined $self->{ClientID}){
	        unshift( @args, $self->{ClientID});
    	}
    	my $resp = $self->send_request('agent.' . $methodName, @args);
        if (ref $resp) {
            if ($resp->is_fault) {
                die "An error occurred: " . $resp->string . "\n";
            } else {
                my $values = $resp->value;
                if (ref($values) eq "HASH") {
                    $hashmap = $values;
                 
                 	my $resp2 = undef;
	                if ($values->{"xpathFound"}) {
	                	my $typeXpathObj = $values->{"xpathFound"};
	                	my $allListeners = $self->{MOBILE_LISTENERS};
	                	my $currentListener = $allListeners->{$typeXpathObj};
	                	if(not defined($currentListener)){
	                    	print STDERR "XPath event was triggered: $typeXpathObj, listener wasn't found";
	                	} else {
	                		my $pos = index($typeXpathObj, ":");
	                		if ($pos >= 0) {
	                            my $type = substr ($typeXpathObj, 0, $pos);
	                            my $xpath = substr ($typeXpathObj, $pos+1);
	                            my $subroutineName = $currentListener->{SUBROUTINE};
	                            my $continueExe = $currentListener->$subroutineName($type, $xpath);
	                            if ($continueExe) {
	                            	$resp2 = $self->send_request('agent.' . $methodName, @args);
									$values = $resp2->value;
	                            } else {
	                            	$self->send_request("agent.disableRecovery");
	                            	die "Recovery $typeXpathObj stop by user";
	                            }
							}
	                	}
	                	$self->send_request("agent.disableRecovery");
	                }                

	                if ($values->{'status'} != 1) {
	                	print STDERR "Status: False";
	                    if ($self->isDieOnFail()) {
	                    	die $values->{'errorMessage'};
	                    } else {
	                    	print STDERR $values->{'errorMessage'};
	                     	return $values;
	                    }
	                 } else {
	                 	# TODO: Return another way
	                 	my $logline = $values->{'logLine'};
	                 	if (defined $logline) {
	                    	print "$logline\n";
	                 	}
	                    return $resp->value;
	                 }
                } else {
                    return {'text' => $values};
                }
            }
        } else {
            print STDERR "Error: Client-side error occured: $resp";
        }
    }
}

sub getLastCommandResultMap() {
    return $hashmap;
}

sub addMobileListener() {
	my ($self, $elementType, $xpath, $packageName, $subroutineName) = @_;
	my $n = scalar @_;
	
	die "Invalid number of arguments. One or more parameters is missing" if (($n==1) || ($n==2) || ($n==4) );
	
	print "too many arguments, only 4 arguments are taken into consideration" if ($n>5);
	
	my $key = "$elementType:$xpath";
	my $listeners = $self->{MOBILE_LISTENERS};

	# not mentioning the package and subroutine names means actually to delete the listener	
	if ($n==3) {
		delete $listeners->{$key};
		$self->execute("removeXPathListener", "$elementType:$xpath");
		return;
	}
				
	die "Missing the subroutine '$subroutineName' in the package '$packageName'"
	unless $packageName->can($subroutineName);

	$listeners->{$key} = bless {CLIENT=>$self, SUBROUTINE=>$subroutineName}, $packageName;
	$self->execute("addXPathListener", $elementType, $xpath);
}

sub pingServer() {
    my ($self) = @_;
    my $response = $self->execute("pingServer");
    if ($self->isDieOnFail()) {
        if (defined $response->{'errorMassage'}) {
            die $response->{'errorMassage'};
        }
    }
}

#        * Description: Command for the Executor Add-On. This command allows you to wait for a device with specific properties, and then execute the test on the device once it's available. The query is build using XPath syntax. For available properties of a specific device, use the GetDeviceInformation command. In order to release the device and make it available for other future tests, use the Release command.
#        *
#        * @param  Search device query.
#        * @param  Timeout in milliseconds to wait for an available device according to the search query.
#        *
#        * This command acts as follows:
#        * 1. Gets a list of all devices that match the user's query.
#        * 2. Filters out devices that are not available (reserved/locked/off-line).
#        * 3. Filters out devices that have already been locked for the current client.
#        * 4. Sorts the list (preferring local devices over remote ones). 
#        * 5. Iterates over the devices list, and over the available agents, until a match is found.
#        * 6. If a match is found, the device is locked and set active, the previous used agent (if any) turns free, and the new one is locked. 
#        * 7. The following applies only in the first call for a specific client: if the current agent does not match the required license, the command tries to lock another agent.
#        * 8. If no match is found or a timeout has passed, an exception is thrown.
#        * The command returns the name of the newly locked device.
#        ** If the device is already locked for the client - it will not be set again. The command will look for an other device.
#        *
#        * @return device name if and only if command succeeded; otherwise an exception is thrown.

sub waitForDevice() {
    my ($self, $query, $timeout) = @_;
	my $currentTime = time;
	my $timeoutInSeconds = $timeout / 1000;
	my $timeoutTime = $currentTime + $timeoutInSeconds;
	
	while (1)
	{
		$currentTime = time;		
		if ($currentTime > $timeoutTime)
		{
            $self->report("waitForDevice - Timeout", 0);
			die "Timeout reached when waiting for device with query: ", $query;
		}
		
		$self->{DIEONFAIL} = 0;
		my $response = $self->execute("lockDevice", RPC::XML::string->new($query));
		$self->{DIEONFAIL} = 1;		
		if ($response->{'status'} == 1)
		{			
            $self->report("wait For " . $query, 1);
			my $deviceName = $response->{'name'};
			return ($deviceName);		
		}
        if ($response->{'validXPath'} and $response->{'validXPath'} == 1)
		{			
			$self->report("waitForDevice - XPath is invalid", 0);
            die "XPath is invalid", $query;
		}
        if ($response->{'license'} and $response->{'license'} == 1)
		{			
			$self->report("waitForDevice - License is not supported on this agent", 0);
            die "XPath is invalid", $query;	
		}
		print "\n";
		CORE::sleep(3);				
	}	
}
sub getVersion() { return "11.3";}

sub activateVoiceAssistance() {
    my ($self, $text) = @_;
    my $response = $self->execute("activateVoiceAssistance", RPC::XML::string->new($text));
}

sub addDevice() {
    my ($self, $serialNumber, $deviceName) = @_;
    my $response = $self->execute("addDevice", RPC::XML::string->new($serialNumber), RPC::XML::string->new($deviceName));
    return $response->{'name'};
}

sub applicationClearData() {
    my ($self, $packageName) = @_;
    my $response = $self->execute("applicationClearData", RPC::XML::string->new($packageName));
}

sub applicationClose() {
    my ($self, $packageName) = @_;
    my $response = $self->execute("applicationClose", RPC::XML::string->new($packageName));
    return $response->{'found'};
}

sub capture() {
    my ($self) = @_;
    my $response = $self->execute("capture");
    return $response->{'outFile'};
}

sub captureLine() {
    my ($self, $line) = @_;
    if (not defined ($line)) {
        $line = "Capture";
    }
    my $response = $self->execute("capture", RPC::XML::string->new($line));
    return $response->{'outFile'};
}

sub captureElement() {
    my ($self, $name, $x, $y, $width, $height, $similarity) = @_;
    if (not defined ($similarity)) {
        $similarity = 97;
    }
    my $response = $self->execute("captureElement", RPC::XML::string->new($name), RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($width), RPC::XML::int->new($height), RPC::XML::int->new($similarity));
}

sub clearAllSms() {
    my ($self) = @_;
    my $response = $self->execute("clearAllSms");
}

sub clearDeviceLog() {
    my ($self) = @_;
    my $response = $self->execute("clearDeviceLog");
}

sub clearLocation() {
    my ($self) = @_;
    my $response = $self->execute("clearLocation");
}

sub click() {
    my ($self, $zone, $element, $index, $clickCount) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    my $response = $self->execute("click", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($clickCount));
}

sub clickOffset() {
    my ($self, $zone, $element, $index, $clickCount, $x, $y) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("click", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($clickCount), RPC::XML::int->new($x), RPC::XML::int->new($y));
}

sub clickCoordinate() {
    my ($self, $x, $y, $clickCount) = @_;
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    my $response = $self->execute("clickCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($clickCount));
}

sub clickIn() {
    my ($self, $zone, $searchElement, $index, $direction, $clickElement, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("clickIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($clickElement), RPC::XML::int->new($width), RPC::XML::int->new($height));
}

sub clickIn2() {
    my ($self, $zone, $searchElement, $index, $direction, $clickElementZone, $clickElement, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("clickIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($clickElementZone), RPC::XML::string->new($clickElement), RPC::XML::int->new($width), RPC::XML::int->new($height));
}

sub clickIn2_5() {
    my ($self, $zone, $searchElement, $index, $direction, $clickElementZone, $clickElement, $width, $height, $clickCount) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    my $response = $self->execute("clickIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($clickElementZone), RPC::XML::string->new($clickElement), RPC::XML::int->new($width), RPC::XML::int->new($height), RPC::XML::int->new($clickCount));
}

sub clickIn3() {
    my ($self, $zone, $searchElement, $index, $direction, $clickElementZone, $clickElement, $clickElementIndex, $width, $height, $clickCount) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($clickElementIndex)) {
        $clickElementIndex = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    my $response = $self->execute("clickIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($clickElementZone), RPC::XML::string->new($clickElement), RPC::XML::int->new($clickElementIndex), RPC::XML::int->new($width), RPC::XML::int->new($height), RPC::XML::int->new($clickCount));
}

sub clickTableCell() {
    my ($self, $zone, $headerElement, $headerIndex, $rowElement, $rowIndex) = @_;
    if (not defined ($headerIndex)) {
        $headerIndex = 0;
    }
    if (not defined ($rowIndex)) {
        $rowIndex = 0;
    }
    my $response = $self->execute("clickTableCell", RPC::XML::string->new($zone), RPC::XML::string->new($headerElement), RPC::XML::int->new($headerIndex), RPC::XML::string->new($rowElement), RPC::XML::int->new($rowIndex));
}

sub closeDevice() {
    my ($self) = @_;
    my $response = $self->execute("closeDevice");
}

sub closeDeviceReflection() {
    my ($self) = @_;
    my $response = $self->execute("closeDeviceReflection");
}

sub closeKeyboard() {
    my ($self) = @_;
    my $response = $self->execute("closeKeyboard");
}

sub collectSupportData() {
    my ($self, $zipDestination, $applicationPath, $device, $scenario, $expectedResult, $actualResult) = @_;
    my $response = $self->execute("collectSupportData", RPC::XML::string->new($zipDestination), RPC::XML::string->new($applicationPath), RPC::XML::string->new($device), RPC::XML::string->new($scenario), RPC::XML::string->new($expectedResult), RPC::XML::string->new($actualResult));
    return $response->{'source'};
}

sub collectSupportData2() {
    my ($self, $zipDestination, $applicationPath, $device, $scenario, $expectedResult, $actualResult, $withCloudData, $onlyLatestLogs) = @_;
    if (not defined ($withCloudData)) {
        $withCloudData = 1;
    }
    if (not defined ($onlyLatestLogs)) {
        $onlyLatestLogs = 1;
    }
    my $response = $self->execute("collectSupportData", RPC::XML::string->new($zipDestination), RPC::XML::string->new($applicationPath), RPC::XML::string->new($device), RPC::XML::string->new($scenario), RPC::XML::string->new($expectedResult), RPC::XML::string->new($actualResult), RPC::XML::boolean->new($withCloudData), RPC::XML::boolean->new($onlyLatestLogs));
    return $response->{'source'};
}

sub deviceAction() {
    my ($self, $action) = @_;
    my $response = $self->execute("deviceAction", RPC::XML::string->new($action));
}

sub drag() {
    my ($self, $zone, $element, $index, $xOffset, $yOffset) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($xOffset)) {
        $xOffset = 0;
    }
    if (not defined ($yOffset)) {
        $yOffset = 0;
    }
    my $response = $self->execute("drag", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($xOffset), RPC::XML::int->new($yOffset));
}

sub dragCoordinates() {
    my ($self, $x1, $y1, $x2, $y2) = @_;
    if (not defined ($x1)) {
        $x1 = 0;
    }
    if (not defined ($y1)) {
        $y1 = 0;
    }
    if (not defined ($x2)) {
        $x2 = 0;
    }
    if (not defined ($y2)) {
        $y2 = 0;
    }
    my $response = $self->execute("dragCoordinates", RPC::XML::int->new($x1), RPC::XML::int->new($y1), RPC::XML::int->new($x2), RPC::XML::int->new($y2));
}

sub dragCoordinates2() {
    my ($self, $x1, $y1, $x2, $y2, $time) = @_;
    if (not defined ($x1)) {
        $x1 = 0;
    }
    if (not defined ($y1)) {
        $y1 = 0;
    }
    if (not defined ($x2)) {
        $x2 = 0;
    }
    if (not defined ($y2)) {
        $y2 = 0;
    }
    if (not defined ($time)) {
        $time = 2000;
    }
    my $response = $self->execute("dragCoordinates", RPC::XML::int->new($x1), RPC::XML::int->new($y1), RPC::XML::int->new($x2), RPC::XML::int->new($y2), RPC::XML::int->new($time));
}

sub dragDrop() {
    my ($self, $zone, $dragElement, $dragIndex, $dropElement, $dropIndex) = @_;
    if (not defined ($dragIndex)) {
        $dragIndex = 0;
    }
    if (not defined ($dropIndex)) {
        $dropIndex = 0;
    }
    my $response = $self->execute("dragDrop", RPC::XML::string->new($zone), RPC::XML::string->new($dragElement), RPC::XML::int->new($dragIndex), RPC::XML::string->new($dropElement), RPC::XML::int->new($dropIndex));
}

sub drop() {
    my ($self) = @_;
    my $response = $self->execute("drop");
}

sub elementGetProperty() {
    my ($self, $zone, $element, $index, $property) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("elementGetProperty", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($property));
    return $response->{'text'};
}

sub elementGetTableRowsCount2() {
    my ($self, $tableLocator, $tableIndex, $visible) = @_;
    if (not defined ($tableIndex)) {
        $tableIndex = 0;
    }
    if (not defined ($visible)) {
        $visible = 0;
    }
    my $response = $self->execute("elementGetTableRowsCount", RPC::XML::string->new($tableLocator), RPC::XML::int->new($tableIndex), RPC::XML::boolean->new($visible));
    return $response->{'count'};
}

sub elementGetTableRowsCount() {
    my ($self, $zone, $tableLocator, $tableIndex, $visible) = @_;
    if (not defined ($tableIndex)) {
        $tableIndex = 0;
    }
    if (not defined ($visible)) {
        $visible = 0;
    }
    my $response = $self->execute("elementGetTableRowsCount", RPC::XML::string->new($zone), RPC::XML::string->new($tableLocator), RPC::XML::int->new($tableIndex), RPC::XML::boolean->new($visible));
    return $response->{'count'};
}

sub elementGetTableValue() {
    my ($self, $rowLocator, $rowLocatorIndex, $columnLocator) = @_;
    if (not defined ($rowLocatorIndex)) {
        $rowLocatorIndex = 0;
    }
    my $response = $self->execute("elementGetTableValue", RPC::XML::string->new($rowLocator), RPC::XML::int->new($rowLocatorIndex), RPC::XML::string->new($columnLocator));
    return $response->{'text'};
}

sub elementGetText() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("elementGetText", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
    return $response->{'text'};
}

sub elementListPick() {
    my ($self, $listZone, $listLocator, $elementZone, $elementLocator, $index, $click) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("elementListPick", RPC::XML::string->new($listZone), RPC::XML::string->new($listLocator), RPC::XML::string->new($elementZone), RPC::XML::string->new($elementLocator), RPC::XML::int->new($index), RPC::XML::boolean->new($click));
}

sub elementListSelect() {
    my ($self, $listLocator, $elementLocator, $index, $click) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("elementListSelect", RPC::XML::string->new($listLocator), RPC::XML::string->new($elementLocator), RPC::XML::int->new($index), RPC::XML::boolean->new($click));
}

sub elementListVisible() {
    my ($self, $listLocator, $elementLocator, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("elementListVisible", RPC::XML::string->new($listLocator), RPC::XML::string->new($elementLocator), RPC::XML::int->new($index));
    return $response->{'found'};
}

sub elementScrollToTableRow2() {
    my ($self, $tableLocator, $tableIndex, $rowIndex) = @_;
    if (not defined ($tableIndex)) {
        $tableIndex = 0;
    }
    if (not defined ($rowIndex)) {
        $rowIndex = 0;
    }
    my $response = $self->execute("elementScrollToTableRow", RPC::XML::string->new($tableLocator), RPC::XML::int->new($tableIndex), RPC::XML::int->new($rowIndex));
}

sub elementScrollToTableRow() {
    my ($self, $zone, $tableLocator, $tableIndex, $rowIndex) = @_;
    if (not defined ($tableIndex)) {
        $tableIndex = 0;
    }
    if (not defined ($rowIndex)) {
        $rowIndex = 0;
    }
    my $response = $self->execute("elementScrollToTableRow", RPC::XML::string->new($zone), RPC::XML::string->new($tableLocator), RPC::XML::int->new($tableIndex), RPC::XML::int->new($rowIndex));
}

sub elementSendText() {
    my ($self, $zone, $element, $index, $text) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("elementSendText", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($text));
}

sub elementSetProperty() {
    my ($self, $zone, $element, $index, $property, $value) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("elementSetProperty", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($property), RPC::XML::string->new($value));
    return $response->{'text'};
}

sub elementSwipe() {
    my ($self, $zone, $element, $index, $direction, $offset, $time) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($time)) {
        $time = 2000;
    }
    my $response = $self->execute("elementSwipe", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($time));
}

sub elementSwipeWhileNotFound() {
    my ($self, $componentZone, $componentElement, $direction, $offset, $swipeTime, $elementfindzone, $elementtofind, $elementtofindindex, $delay, $rounds, $click) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($swipeTime)) {
        $swipeTime = 2000;
    }
    if (not defined ($elementtofindindex)) {
        $elementtofindindex = 0;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    if (not defined ($rounds)) {
        $rounds = 5;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("elementSwipeWhileNotFound", RPC::XML::string->new($componentZone), RPC::XML::string->new($componentElement), RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($swipeTime), RPC::XML::string->new($elementfindzone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($elementtofindindex), RPC::XML::int->new($delay), RPC::XML::int->new($rounds), RPC::XML::boolean->new($click));
    return $response->{'found'};
}

sub endTransaction() {
    my ($self, $name) = @_;
    my $response = $self->execute("endTransaction", RPC::XML::string->new($name));
}

sub exit() {
    my ($self) = @_;
    my $response = $self->execute("exit");
}

sub extractLanguageFiles() {
    my ($self, $application, $directoryPath, $allowOverwrite) = @_;
    if (not defined ($allowOverwrite)) {
        $allowOverwrite = 1;
    }
    my $response = $self->execute("extractLanguageFiles", RPC::XML::string->new($application), RPC::XML::string->new($directoryPath), RPC::XML::boolean->new($allowOverwrite));
}

sub findElements() {
    my ($self, $zone, $parent, $by, $value) = @_;
    my $response = $self->execute("findElements", RPC::XML::string->new($zone), RPC::XML::string->new($parent), RPC::XML::string->new($by), RPC::XML::string->new($value));
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub flick() {
    my ($self, $direction, $offset) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    my $response = $self->execute("flick", RPC::XML::string->new($direction), RPC::XML::int->new($offset));
}

sub flickCoordinate() {
    my ($self, $x, $y, $direction) = @_;
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("flickCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::string->new($direction));
}

sub flickElement() {
    my ($self, $zone, $element, $index, $direction) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("flickElement", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($direction));
}

sub forceTouch() {
    my ($self, $zone, $element, $index, $duration, $force, $dragDistanceX, $dragDistanceY, $dragDuration) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($duration)) {
        $duration = 100;
    }
    if (not defined ($force)) {
        $force = 100;
    }
    if (not defined ($dragDistanceX)) {
        $dragDistanceX = 0;
    }
    if (not defined ($dragDistanceY)) {
        $dragDistanceY = 0;
    }
    if (not defined ($dragDuration)) {
        $dragDuration = 1500;
    }
    my $response = $self->execute("forceTouch", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($duration), RPC::XML::int->new($force), RPC::XML::int->new($dragDistanceX), RPC::XML::int->new($dragDistanceY), RPC::XML::int->new($dragDuration));
}

sub generateReport() {
    my ($self) = @_;
    my $response = $self->execute("generateReport");
    return $response->{'text'};
}

sub generateReport2() {
    my ($self, $releaseClient) = @_;
    if (not defined ($releaseClient)) {
        $releaseClient = 1;
    }
    my $response = $self->execute("generateReport", RPC::XML::boolean->new($releaseClient));
    return $response->{'text'};
}

sub generateReport() {
    my ($self, $releaseClient, $propFilePath) = @_;
    if (not defined ($releaseClient)) {
        $releaseClient = 1;
    }
    my $response = $self->execute("generateReport", RPC::XML::boolean->new($releaseClient), RPC::XML::string->new($propFilePath));
    return $response->{'text'};
}

sub getAllSms() {
    my ($self, $timeout) = @_;
    if (not defined ($timeout)) {
        $timeout = 5000;
    }
    my $response = $self->execute("getAllSms", RPC::XML::int->new($timeout));
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getAllValues() {
    my ($self, $zone, $element, $property) = @_;
    my $response = $self->execute("getAllValues", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::string->new($property));
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getAllZonesWithElement() {
    my ($self, $element) = @_;
    my $response = $self->execute("getAllZonesWithElement", RPC::XML::string->new($element));
    return $response->{'text'};
}

sub getAvailableAgentPort() {
    my ($self) = @_;
    my $response = $self->execute("getAvailableAgentPort");
    return $response->{'port'};
}

sub getAvailableAgentPort2() {
    my ($self, $featureName) = @_;
    my $response = $self->execute("getAvailableAgentPort", RPC::XML::string->new($featureName));
    return $response->{'port'};
}

sub getBrowserTabIdList() {
    my ($self) = @_;
    my $response = $self->execute("getBrowserTabIdList");
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getConnectedDevices() {
    my ($self) = @_;
    my $response = $self->execute("getConnectedDevices");
    return $response->{'text'};
}

sub getContextList() {
    my ($self) = @_;
    my $response = $self->execute("getContextList");
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getCoordinateColor() {
    my ($self, $x, $y) = @_;
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("getCoordinateColor", RPC::XML::int->new($x), RPC::XML::int->new($y));
    return $response->{'color'};
}

sub getCounter() {
    my ($self, $counterName) = @_;
    my $response = $self->execute("getCounter", RPC::XML::string->new($counterName));
    return $response->{'text'};
}

sub getCurrentApplicationName() {
    my ($self) = @_;
    my $response = $self->execute("getCurrentApplicationName");
    return $response->{'text'};
}

sub getCurrentBrowserTabId() {
    my ($self) = @_;
    my $response = $self->execute("getCurrentBrowserTabId");
    return $response->{'text'};
}

sub getDefaultTimeout() {
    my ($self) = @_;
    my $response = $self->execute("getDefaultTimeout");
    return $response->{'timeout'};
}

sub getDeviceLog() {
    my ($self) = @_;
    my $response = $self->execute("getDeviceLog");
    return $response->{'path'};
}

sub getDeviceProperty() {
    my ($self, $key) = @_;
    my $response = $self->execute("getDeviceProperty", RPC::XML::string->new($key));
    return $response->{'text'};
}

sub getDevicesInformation() {
    my ($self) = @_;
    my $response = $self->execute("getDevicesInformation");
    return $response->{'text'};
}

sub getElementCount() {
    my ($self, $zone, $element) = @_;
    my $response = $self->execute("getElementCount", RPC::XML::string->new($zone), RPC::XML::string->new($element));
    return $response->{'count'};
}

sub getElementCountIn() {
    my ($self, $zoneName, $elementSearch, $index, $direction, $elementCountZone, $elementCount, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("getElementCountIn", RPC::XML::string->new($zoneName), RPC::XML::string->new($elementSearch), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($elementCountZone), RPC::XML::string->new($elementCount), RPC::XML::int->new($width), RPC::XML::int->new($height));
    return $response->{'count'};
}

sub getInstalledApplications() {
    my ($self) = @_;
    my $response = $self->execute("getInstalledApplications");
    return $response->{'text'};
}

sub getLastSMS() {
    my ($self, $timeout) = @_;
    if (not defined ($timeout)) {
        $timeout = 5000;
    }
    my $response = $self->execute("getLastSMS", RPC::XML::int->new($timeout));
    return $response->{'text'};
}

sub getMonitorsData() {
    my ($self, $cSVfilepath) = @_;
    my $response = $self->execute("getMonitorsData", RPC::XML::string->new($cSVfilepath));
    return $response->{'text'};
}

sub getNetworkConnection() {
    my ($self, $connection) = @_;
    my $response = $self->execute("getNetworkConnection", RPC::XML::string->new($connection));
    return $response->{'found'};
}

sub getNVProfiles() {
    my ($self) = @_;
    my $response = $self->execute("getNVProfiles");
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getPickerValues() {
    my ($self, $zone, $pickerElement, $index, $wheelIndex) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($wheelIndex)) {
        $wheelIndex = 0;
    }
    my $response = $self->execute("getPickerValues", RPC::XML::string->new($zone), RPC::XML::string->new($pickerElement), RPC::XML::int->new($index), RPC::XML::int->new($wheelIndex));
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getPosition() {
    my ($self, $zone, $element) = @_;
    my $response = $self->execute("getPosition", RPC::XML::string->new($zone), RPC::XML::string->new($element));
    return $response->{'click'};
}

sub getPositionWindowRelative() {
    my ($self, $zone, $element) = @_;
    my $response = $self->execute("getPositionWindowRelative", RPC::XML::string->new($zone), RPC::XML::string->new($element));
    return $response->{'centerWinRelative'};
}

sub getProperty() {
    my ($self, $property) = @_;
    my $response = $self->execute("getProperty", RPC::XML::string->new($property));
    return $response->{'text'};
}

sub getSimCard() {
    my ($self) = @_;
    my $response = $self->execute("getSimCard");
    return $response->{'text'};
}

sub getSimCards() {
    my ($self, $readyToUse) = @_;
    if (not defined ($readyToUse)) {
        $readyToUse = 1;
    }
    my $response = $self->execute("getSimCards", RPC::XML::boolean->new($readyToUse));
    my @textArray=@{$response->{'textArray'}};
    return @textArray;
}

sub getTableCellText() {
    my ($self, $zone, $headerElement, $headerIndex, $rowElement, $rowIndex, $width, $height) = @_;
    if (not defined ($headerIndex)) {
        $headerIndex = 0;
    }
    if (not defined ($rowIndex)) {
        $rowIndex = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("getTableCellText", RPC::XML::string->new($zone), RPC::XML::string->new($headerElement), RPC::XML::int->new($headerIndex), RPC::XML::string->new($rowElement), RPC::XML::int->new($rowIndex), RPC::XML::int->new($width), RPC::XML::int->new($height));
    return $response->{'text'};
}

sub getText() {
    my ($self, $zone) = @_;
    my $response = $self->execute("getText", RPC::XML::string->new($zone));
    return $response->{'text'};
}

sub getTextIn() {
    my ($self, $zone, $element, $index, $direction, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("getTextIn", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::int->new($width), RPC::XML::int->new($height));
    return $response->{'text'};
}

sub getTextIn2() {
    my ($self, $zone, $element, $index, $textZone, $direction, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("getTextIn", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($textZone), RPC::XML::string->new($direction), RPC::XML::int->new($width), RPC::XML::int->new($height));
    return $response->{'text'};
}

sub getTextIn3() {
    my ($self, $zone, $element, $index, $textZone, $direction, $width, $height, $xOffset, $yOffset) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    if (not defined ($xOffset)) {
        $xOffset = 0;
    }
    if (not defined ($yOffset)) {
        $yOffset = 0;
    }
    my $response = $self->execute("getTextIn", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($textZone), RPC::XML::string->new($direction), RPC::XML::int->new($width), RPC::XML::int->new($height), RPC::XML::int->new($xOffset), RPC::XML::int->new($yOffset));
    return $response->{'text'};
}

sub getVisualDump() {
    my ($self, $type) = @_;
    if (not defined ($type)) {
        $type = "Native";
    }
    my $response = $self->execute("getVisualDump", RPC::XML::string->new($type));
    return $response->{'text'};
}

sub hybridClearCache() {
    my ($self, $clearCookies, $clearCache) = @_;
    if (not defined ($clearCookies)) {
        $clearCookies = 1;
    }
    if (not defined ($clearCache)) {
        $clearCache = 1;
    }
    my $response = $self->execute("hybridClearCache", RPC::XML::boolean->new($clearCookies), RPC::XML::boolean->new($clearCache));
    if ($self->isDieOnFail()) {
        if (defined $response->{'errorMassage'}) {
            die $response->{'errorMassage'};
        }
    }
}

sub hybridGetHtml() {
    my ($self, $webViewLocator, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("hybridGetHtml", RPC::XML::string->new($webViewLocator), RPC::XML::int->new($index));
    return $response->{'text'};
}

sub hybridRunJavascript() {
    my ($self, $webViewLocator, $index, $script) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("hybridRunJavascript", RPC::XML::string->new($webViewLocator), RPC::XML::int->new($index), RPC::XML::string->new($script));
    return $response->{'text'};
}

sub hybridSelect() {
    my ($self, $webViewLocator, $index, $method, $value, $select) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("hybridSelect", RPC::XML::string->new($webViewLocator), RPC::XML::int->new($index), RPC::XML::string->new($method), RPC::XML::string->new($value), RPC::XML::string->new($select));
}

sub hybridWaitForPageLoad() {
    my ($self, $timeout) = @_;
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("hybridWaitForPageLoad", RPC::XML::int->new($timeout));
}

sub install() {
    my ($self, $path, $sign) = @_;
    if (not defined ($sign)) {
        $sign = 0;
    }
    my $response = $self->execute("install", RPC::XML::string->new($path), RPC::XML::boolean->new($sign));
    return $response->{'found'};
}

sub install2() {
    my ($self, $path, $instrument, $keepData) = @_;
    if (not defined ($instrument)) {
        $instrument = 1;
    }
    if (not defined ($keepData)) {
        $keepData = 0;
    }
    my $response = $self->execute("install", RPC::XML::string->new($path), RPC::XML::boolean->new($instrument), RPC::XML::boolean->new($keepData));
    return $response->{'found'};
}

sub installWithCustomKeystore() {
    my ($self, $path, $instrument, $keepData, $keystorePath, $keystorePassword, $keyAlias, $keyPassword) = @_;
    if (not defined ($instrument)) {
        $instrument = 1;
    }
    if (not defined ($keepData)) {
        $keepData = 0;
    }
    my $response = $self->execute("installWithCustomKeystore", RPC::XML::string->new($path), RPC::XML::boolean->new($instrument), RPC::XML::boolean->new($keepData), RPC::XML::string->new($keystorePath), RPC::XML::string->new($keystorePassword), RPC::XML::string->new($keyAlias), RPC::XML::string->new($keyPassword));
    return $response->{'found'};
}

sub isElementBlank() {
    my ($self, $zone, $element, $index, $colorGroups) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($colorGroups)) {
        $colorGroups = 10;
    }
    my $response = $self->execute("isElementBlank", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($colorGroups));
    return $response->{'found'};
}

sub isElementFound2() {
    my ($self, $zone, $element) = @_;
    my $response = $self->execute("isElementFound", RPC::XML::string->new($zone), RPC::XML::string->new($element));
    return $response->{'found'};
}

sub isElementFound() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("isElementFound", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
    return $response->{'found'};
}

sub isFoundIn() {
    my ($self, $zone, $searchElement, $index, $direction, $elementFindZone, $elementToFind, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("isFoundIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($elementFindZone), RPC::XML::string->new($elementToFind), RPC::XML::int->new($width), RPC::XML::int->new($height));
    return $response->{'found'};
}

sub isTextFound() {
    my ($self, $zone, $element, $ignoreCase) = @_;
    my $response = $self->execute("isTextFound", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::boolean->new($ignoreCase));
    return $response->{'found'};
}

sub launch2() {
    my ($self, $applicationIdentifier, $launchOptions) = @_;
    my $response = $self->execute("launch", RPC::XML::string->new($applicationIdentifier), RPC::XML::dict->new($launchOptions));
}

sub launch() {
    my ($self, $activityURL, $instrument, $stopIfRunning) = @_;
    if (not defined ($instrument)) {
        $instrument = 1;
    }
    if (not defined ($stopIfRunning)) {
        $stopIfRunning = 0;
    }
    my $response = $self->execute("launch", RPC::XML::string->new($activityURL), RPC::XML::boolean->new($instrument), RPC::XML::boolean->new($stopIfRunning));
}

sub listSelect() {
    my ($self, $sendRest, $sendNavigation, $delay, $textToIdentify, $color, $rounds, $sendonfind) = @_;
    if (not defined ($sendRest)) {
        $sendRest = "{UP}";
    }
    if (not defined ($sendNavigation)) {
        $sendNavigation = "{DOWN}";
    }
    if (not defined ($delay)) {
        $delay = 500;
    }
    if (not defined ($rounds)) {
        $rounds = 5;
    }
    if (not defined ($sendonfind)) {
        $sendonfind = "{ENTER}";
    }
    my $response = $self->execute("listSelect", RPC::XML::string->new($sendRest), RPC::XML::string->new($sendNavigation), RPC::XML::int->new($delay), RPC::XML::string->new($textToIdentify), RPC::XML::string->new($color), RPC::XML::int->new($rounds), RPC::XML::string->new($sendonfind));
    return $response->{'found'};
}

sub longClick() {
    my ($self, $zone, $element, $index, $clickCount, $x, $y) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("longClick", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($clickCount), RPC::XML::int->new($x), RPC::XML::int->new($y));
}

sub maximize() {
    my ($self) = @_;
    my $response = $self->execute("maximize");
}

sub multiClick() {
    my ($self, $zone, $element, $index, $fingerIndex) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("multiClick", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($fingerIndex));
}

sub multiClickCoordinate() {
    my ($self, $x, $y, $fingerIndex) = @_;
    my $response = $self->execute("multiClickCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($fingerIndex));
}

sub multiClickOffset() {
    my ($self, $zone, $element, $index, $x, $y, $fingerIndex) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("multiClickOffset", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($fingerIndex));
}

sub multiSwipe() {
    my ($self, $direction, $offset, $time, $fingerIndex) = @_;
    if (not defined ($time)) {
        $time = 500;
    }
    my $response = $self->execute("multiSwipe", RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($time), RPC::XML::int->new($fingerIndex));
}

sub multiTouchDown() {
    my ($self, $zone, $element, $index, $fingerIndex) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("multiTouchDown", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($fingerIndex));
}

sub multiTouchDownCoordinate() {
    my ($self, $x, $y, $fingerIndex) = @_;
    my $response = $self->execute("multiTouchDownCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($fingerIndex));
}

sub multiTouchMove() {
    my ($self, $zone, $element, $index, $fingerIndex) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("multiTouchMove", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($fingerIndex));
}

sub multiTouchMoveCoordinate() {
    my ($self, $x, $y, $fingerIndex) = @_;
    my $response = $self->execute("multiTouchMoveCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($fingerIndex));
}

sub multiTouchUp() {
    my ($self, $fingerIndex) = @_;
    my $response = $self->execute("multiTouchUp", RPC::XML::int->new($fingerIndex));
}

sub multiWait() {
    my ($self, $time, $fingerIndex) = @_;
    if (not defined ($time)) {
        $time = 500;
    }
    my $response = $self->execute("multiWait", RPC::XML::int->new($time), RPC::XML::int->new($fingerIndex));
}

sub openDevice() {
    my ($self) = @_;
    my $response = $self->execute("openDevice");
}

sub p2cx() {
    my ($self, $percentage) = @_;
    if (not defined ($percentage)) {
        $percentage = 0;
    }
    my $response = $self->execute("p2cx", RPC::XML::int->new($percentage));
    return $response->{'pixel'};
}

sub p2cy() {
    my ($self, $percentage) = @_;
    if (not defined ($percentage)) {
        $percentage = 0;
    }
    my $response = $self->execute("p2cy", RPC::XML::int->new($percentage));
    return $response->{'pixel'};
}

sub performMultiGesture() {
    my ($self) = @_;
    my $response = $self->execute("performMultiGesture");
}

sub pinch() {
    my ($self, $inside, $x, $y, $radius) = @_;
    if (not defined ($inside)) {
        $inside = 1;
    }
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    if (not defined ($radius)) {
        $radius = 100;
    }
    my $response = $self->execute("pinch", RPC::XML::boolean->new($inside), RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($radius));
    return $response->{'found'};
}

sub pinch2() {
    my ($self, $inside, $x, $y, $radius, $horizontal) = @_;
    if (not defined ($inside)) {
        $inside = 1;
    }
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    if (not defined ($radius)) {
        $radius = 100;
    }
    if (not defined ($horizontal)) {
        $horizontal = 0;
    }
    my $response = $self->execute("pinch", RPC::XML::boolean->new($inside), RPC::XML::int->new($x), RPC::XML::int->new($y), RPC::XML::int->new($radius), RPC::XML::boolean->new($horizontal));
    return $response->{'found'};
}

sub portForward() {
    my ($self, $localPort, $remotePort) = @_;
    my $response = $self->execute("portForward", RPC::XML::int->new($localPort), RPC::XML::int->new($remotePort));
    return $response->{'port'};
}

sub pressWhileNotFound() {
    my ($self, $zone, $elementtoclick, $elementtofind, $timeout, $delay) = @_;
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    if (not defined ($delay)) {
        $delay = 0;
    }
    my $response = $self->execute("pressWhileNotFound", RPC::XML::string->new($zone), RPC::XML::string->new($elementtoclick), RPC::XML::string->new($elementtofind), RPC::XML::int->new($timeout), RPC::XML::int->new($delay));
}

sub pressWhileNotFound2() {
    my ($self, $zone, $elementtoclick, $elementtoclickindex, $elementtofind, $elementtofindindex, $timeout, $delay) = @_;
    if (not defined ($elementtoclickindex)) {
        $elementtoclickindex = 0;
    }
    if (not defined ($elementtofindindex)) {
        $elementtofindindex = 0;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    if (not defined ($delay)) {
        $delay = 0;
    }
    my $response = $self->execute("pressWhileNotFound", RPC::XML::string->new($zone), RPC::XML::string->new($elementtoclick), RPC::XML::int->new($elementtoclickindex), RPC::XML::string->new($elementtofind), RPC::XML::int->new($elementtofindindex), RPC::XML::int->new($timeout), RPC::XML::int->new($delay));
}

sub reboot() {
    my ($self, $timeout) = @_;
    if (not defined ($timeout)) {
        $timeout = 120000;
    }
    my $response = $self->execute("reboot", RPC::XML::int->new($timeout));
    return $response->{'status'};
}

sub receiveIncomingCall() {
    my ($self, $fromNumber, $hangupInSeconds) = @_;
    my $response = $self->execute("receiveIncomingCall", RPC::XML::string->new($fromNumber), RPC::XML::int->new($hangupInSeconds));
}

sub receiveIncomingSMS() {
    my ($self, $fromNumber, $msg) = @_;
    my $response = $self->execute("receiveIncomingSMS", RPC::XML::string->new($fromNumber), RPC::XML::string->new($msg));
}

sub releaseClient() {
    my ($self) = @_;
    my $response = $self->execute("releaseClient");
}

sub releaseDevice() {
    my ($self, $deviceName, $releaseAgent, $removeFromDeviceList, $releaseFromCloud) = @_;
    if (not defined ($releaseAgent)) {
        $releaseAgent = 1;
    }
    if (not defined ($removeFromDeviceList)) {
        $removeFromDeviceList = 0;
    }
    if (not defined ($releaseFromCloud)) {
        $releaseFromCloud = 1;
    }
    my $response = $self->execute("releaseDevice", RPC::XML::string->new($deviceName), RPC::XML::boolean->new($releaseAgent), RPC::XML::boolean->new($removeFromDeviceList), RPC::XML::boolean->new($releaseFromCloud));
}

sub report() {
    my ($self, $message, $status) = @_;
    my $response = $self->execute("report", RPC::XML::string->new($message), RPC::XML::boolean->new($status));
}

sub reportWithImage() {
    my ($self, $pathToImage, $message, $status) = @_;
    my $response = $self->execute("report", RPC::XML::string->new($pathToImage), RPC::XML::string->new($message), RPC::XML::boolean->new($status));
}

sub resetDeviceBridge() {
    my ($self) = @_;
    my $response = $self->execute("resetDeviceBridge");
}

sub resetDeviceBridgeOS() {
    my ($self, $deviceType) = @_;
    my $response = $self->execute("resetDeviceBridge", RPC::XML::string->new($deviceType));
}

sub rightClick() {
    my ($self, $zone, $element, $index, $clickCount, $x, $y) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($clickCount)) {
        $clickCount = 1;
    }
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("rightClick", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($clickCount), RPC::XML::int->new($x), RPC::XML::int->new($y));
}

sub run() {
    my ($self, $command) = @_;
    my $response = $self->execute("run", RPC::XML::string->new($command));
    return $response->{'text'};
}

sub LayoutTest() {
    my ($self, $xml) = @_;
    my $response = $self->execute("runLayoutTest", RPC::XML::string->new($xml));
    return $response->{'text'};
}

sub runNativeAPICall() {
    my ($self, $zone, $element, $index, $script) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("runNativeAPICall", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::string->new($script));
    return $response->{'text'};
}

sub sendText() {
    my ($self, $text) = @_;
    my $response = $self->execute("sendText", RPC::XML::string->new($text));
}

sub sendWhileNotFound() {
    my ($self, $toSend, $zone, $elementtofind, $timeout, $delay) = @_;
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    my $response = $self->execute("sendWhileNotFound", RPC::XML::string->new($toSend), RPC::XML::string->new($zone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($timeout), RPC::XML::int->new($delay));
}

sub sendWhileNotFound2() {
    my ($self, $toSend, $zone, $elementtofind, $elementtofindindex, $timeout, $delay) = @_;
    if (not defined ($elementtofindindex)) {
        $elementtofindindex = 0;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    my $response = $self->execute("sendWhileNotFound", RPC::XML::string->new($toSend), RPC::XML::string->new($zone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($elementtofindindex), RPC::XML::int->new($timeout), RPC::XML::int->new($delay));
}

sub setApplicationTitle() {
    my ($self, $title) = @_;
    my $response = $self->execute("setApplicationTitle", RPC::XML::string->new($title));
}

sub setAuthenticationReply() {
    my ($self, $reply, $delay) = @_;
    if (not defined ($reply)) {
        $reply = "Success";
    }
    if (not defined ($delay)) {
        $delay = 0;
    }
    my $response = $self->execute("setAuthenticationReply", RPC::XML::string->new($reply), RPC::XML::int->new($delay));
}

sub setContext() {
    my ($self, $context) = @_;
    if (not defined ($context)) {
        $context = "NATIVE_APP";
    }
    my $response = $self->execute("setContext", RPC::XML::string->new($context));
}

sub setDefaultClickDownTime() {
    my ($self, $downTime) = @_;
    if (not defined ($downTime)) {
        $downTime = 100;
    }
    my $response = $self->execute("setDefaultClickDownTime", RPC::XML::int->new($downTime));
}

sub setDefaultTimeout() {
    my ($self, $newTimeout) = @_;
    if (not defined ($newTimeout)) {
        $newTimeout = 20000;
    }
    my $response = $self->execute("setDefaultTimeout", RPC::XML::int->new($newTimeout));
    return $response->{'timeout'};
}

sub setDefaultWebView() {
    my ($self, $webViewLocator) = @_;
    my $response = $self->execute("setDefaultWebView", RPC::XML::string->new($webViewLocator));
}

sub setDevice() {
    my ($self, $device) = @_;
    my $response = $self->execute("setDevice", RPC::XML::string->new($device));
}

sub setDragStartDelay() {
    my ($self, $delay) = @_;
    if (not defined ($delay)) {
        $delay = 0;
    }
    my $response = $self->execute("setDragStartDelay", RPC::XML::int->new($delay));
}

sub setInKeyDelay() {
    my ($self, $delay) = @_;
    if (not defined ($delay)) {
        $delay = 50;
    }
    my $response = $self->execute("setInKeyDelay", RPC::XML::int->new($delay));
}

sub setKeyToKeyDelay() {
    my ($self, $delay) = @_;
    if (not defined ($delay)) {
        $delay = 50;
    }
    my $response = $self->execute("setKeyToKeyDelay", RPC::XML::int->new($delay));
}

sub setLanguage() {
    my ($self, $language) = @_;
    my $response = $self->execute("setLanguage", RPC::XML::string->new($language));
}

sub setLanguagePropertiesFile() {
    my ($self, $propertiesfile) = @_;
    my $response = $self->execute("setLanguagePropertiesFile", RPC::XML::string->new($propertiesfile));
}

sub setLocation() {
    my ($self, $latitude, $longitude) = @_;
    if (not defined ($latitude)) {
        $latitude = "0.0";
    }
    if (not defined ($longitude)) {
        $longitude = "0.0";
    }
    my $response = $self->execute("setLocation", RPC::XML::string->new($latitude), RPC::XML::string->new($longitude));
}

sub setMonitorPollingInterval() {
    my ($self, $timemilli) = @_;
    if (not defined ($timemilli)) {
        $timemilli = 30000;
    }
    my $response = $self->execute("setMonitorPollingInterval", RPC::XML::int->new($timemilli));
}

sub setMonitorTestState() {
    my ($self, $testStatus) = @_;
    my $response = $self->execute("setMonitorTestState", RPC::XML::string->new($testStatus));
}

sub setNetworkConditions() {
    my ($self, $profile) = @_;
    my $response = $self->execute("setNetworkConditions", RPC::XML::string->new($profile));
}

sub setNetworkConditions2() {
    my ($self, $profile, $duration) = @_;
    if (not defined ($duration)) {
        $duration = 0;
    }
    my $response = $self->execute("setNetworkConditions", RPC::XML::string->new($profile), RPC::XML::int->new($duration));
}

sub setNetworkConnection() {
    my ($self, $connection, $enable) = @_;
    my $response = $self->execute("setNetworkConnection", RPC::XML::string->new($connection), RPC::XML::boolean->new($enable));
}

sub setOcrIgnoreCase() {
    my ($self, $ignoreCase) = @_;
    my $response = $self->execute("setOcrIgnoreCase", RPC::XML::boolean->new($ignoreCase));
}

sub setOcrTrainingFilePath() {
    my ($self, $trainingPath) = @_;
    my $response = $self->execute("setOcrTrainingFilePath", RPC::XML::string->new($trainingPath));
}

sub setPickerValues() {
    my ($self, $zoneName, $elementName, $index, $wheelIndex, $value) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("setPickerValues", RPC::XML::string->new($zoneName), RPC::XML::string->new($elementName), RPC::XML::int->new($index), RPC::XML::int->new($wheelIndex), RPC::XML::string->new($value));
    return $response->{'text'};
}

sub setProjectBaseDirectory() {
    my ($self, $projectBaseDirectory) = @_;
    my $response = $self->execute("setProjectBaseDirectory", RPC::XML::string->new($projectBaseDirectory));
}

sub setProperty() {
    my ($self, $key, $value) = @_;
    my $response = $self->execute("setProperty", RPC::XML::string->new($key), RPC::XML::string->new($value));
}

sub setRedToBlue() {
    my ($self, $redToBlue) = @_;
    my $response = $self->execute("setRedToBlue", RPC::XML::boolean->new($redToBlue));
}

sub setReporter() {
    my ($self, $reporterName, $directory) = @_;
    if (not defined ($reporterName)) {
        $reporterName = "html";
    }
    my $response = $self->execute("setReporter", RPC::XML::string->new($reporterName), RPC::XML::string->new($directory));
    return $response->{'text'};
}

sub setReporter2() {
    my ($self, $reporterName, $directory, $testName) = @_;
    if (not defined ($reporterName)) {
        $reporterName = "html";
    }
    my $response = $self->execute("setReporter", RPC::XML::string->new($reporterName), RPC::XML::string->new($directory), RPC::XML::string->new($testName));
    return $response->{'text'};
}

sub setShowImageAsLink() {
    my ($self, $showImageAsLink) = @_;
    my $response = $self->execute("setShowImageAsLink", RPC::XML::boolean->new($showImageAsLink));
}

sub setShowImageInReport() {
    my ($self, $showImageInReport) = @_;
    if (not defined ($showImageInReport)) {
        $showImageInReport = 1;
    }
    my $response = $self->execute("setShowImageInReport", RPC::XML::boolean->new($showImageInReport));
}

sub setShowPassImageInReport() {
    my ($self, $showPassImageInReport) = @_;
    if (not defined ($showPassImageInReport)) {
        $showPassImageInReport = 1;
    }
    my $response = $self->execute("setShowPassImageInReport", RPC::XML::boolean->new($showPassImageInReport));
}

sub setShowReport() {
    my ($self, $showReport) = @_;
    if (not defined ($showReport)) {
        $showReport = 1;
    }
    my $response = $self->execute("setShowReport", RPC::XML::boolean->new($showReport));
}

sub setSimCard() {
    my ($self, $simCardName) = @_;
    my $response = $self->execute("setSimCard", RPC::XML::string->new($simCardName));
}

sub setSpeed() {
    my ($self, $speed) = @_;
    my $response = $self->execute("setSpeed", RPC::XML::string->new($speed));
}

sub setTestStatus() {
    my ($self, $status, $message) = @_;
    if (not defined ($status)) {
        $status = 1;
    }
    my $response = $self->execute("setTestStatus", RPC::XML::boolean->new($status), RPC::XML::string->new($message));
}

sub setWebAutoScroll() {
    my ($self, $autoScroll) = @_;
    if (not defined ($autoScroll)) {
        $autoScroll = 1;
    }
    my $response = $self->execute("setWebAutoScroll", RPC::XML::boolean->new($autoScroll));
}

sub setWindowSize() {
    my ($self, $width, $height) = @_;
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("setWindowSize", RPC::XML::int->new($width), RPC::XML::int->new($height));
}

sub shake() {
    my ($self) = @_;
    my $response = $self->execute("shake");
}

sub simulateCapture() {
    my ($self, $picturePath) = @_;
    my $response = $self->execute("simulateCapture", RPC::XML::string->new($picturePath));
}

sub sleep() {
    my ($self, $time) = @_;
    if (not defined ($time)) {
        $time = 1000;
    }
    my $response = $self->execute("sleep", RPC::XML::int->new($time));
}

sub startAudioPlay() {
    my ($self, $audioFile) = @_;
    my $response = $self->execute("startAudioPlay", RPC::XML::string->new($audioFile));
}

sub startAudioRecording() {
    my ($self, $audioFile) = @_;
    my $response = $self->execute("startAudioRecording", RPC::XML::string->new($audioFile));
}

sub startCall() {
    my ($self, $skypeUser, $skypePassword, $number, $duration) = @_;
    if (not defined ($duration)) {
        $duration = 0;
    }
    my $response = $self->execute("startCall", RPC::XML::string->new($skypeUser), RPC::XML::string->new($skypePassword), RPC::XML::string->new($number), RPC::XML::int->new($duration));
}

sub startLoggingDevice() {
    my ($self, $path) = @_;
    my $response = $self->execute("startLoggingDevice", RPC::XML::string->new($path));
}

sub startMonitor() {
    my ($self, $packageName) = @_;
    my $response = $self->execute("startMonitor", RPC::XML::string->new($packageName));
}

sub startMultiGesture() {
    my ($self, $name) = @_;
    my $response = $self->execute("startMultiGesture", RPC::XML::string->new($name));
}

sub startStepsGroup() {
    my ($self, $caption) = @_;
    my $response = $self->execute("startStepsGroup", RPC::XML::string->new($caption));
}

sub startTransaction() {
    my ($self, $name) = @_;
    my $response = $self->execute("startTransaction", RPC::XML::string->new($name));
}

sub startVideoRecord() {
    my ($self) = @_;
    my $response = $self->execute("startVideoRecord");
}

sub stopAudioPlay() {
    my ($self) = @_;
    my $response = $self->execute("stopAudioPlay");
}

sub stopAudioRecording() {
    my ($self) = @_;
    my $response = $self->execute("stopAudioRecording");
}

sub stopLoggingDevice() {
    my ($self) = @_;
    my $response = $self->execute("stopLoggingDevice");
    return $response->{'text'};
}

sub stopStepsGroup() {
    my ($self) = @_;
    my $response = $self->execute("stopStepsGroup");
}

sub stopVideoRecord() {
    my ($self) = @_;
    my $response = $self->execute("stopVideoRecord");
    return $response->{'text'};
}

sub swipe() {
    my ($self, $direction, $offset) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    my $response = $self->execute("swipe", RPC::XML::string->new($direction), RPC::XML::int->new($offset));
}

sub swipe2() {
    my ($self, $direction, $offset, $time) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($time)) {
        $time = 500;
    }
    my $response = $self->execute("swipe", RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($time));
}

sub swipeWhileNotFound() {
    my ($self, $direction, $offset, $zone, $elementtofind, $delay, $rounds, $click) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    if (not defined ($rounds)) {
        $rounds = 5;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("swipeWhileNotFound", RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::string->new($zone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($delay), RPC::XML::int->new($rounds), RPC::XML::boolean->new($click));
    return $response->{'found'};
}

sub swipeWhileNotFound3() {
    my ($self, $direction, $offset, $swipeTime, $zone, $elementtofind, $delay, $rounds, $click) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($swipeTime)) {
        $swipeTime = 2000;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    if (not defined ($rounds)) {
        $rounds = 5;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("swipeWhileNotFound", RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($swipeTime), RPC::XML::string->new($zone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($delay), RPC::XML::int->new($rounds), RPC::XML::boolean->new($click));
    return $response->{'found'};
}

sub swipeWhileNotFound2() {
    my ($self, $direction, $offset, $swipeTime, $zone, $elementtofind, $elementtofindindex, $delay, $rounds, $click) = @_;
    if (not defined ($offset)) {
        $offset = 0;
    }
    if (not defined ($swipeTime)) {
        $swipeTime = 2000;
    }
    if (not defined ($elementtofindindex)) {
        $elementtofindindex = 0;
    }
    if (not defined ($delay)) {
        $delay = 1000;
    }
    if (not defined ($rounds)) {
        $rounds = 5;
    }
    if (not defined ($click)) {
        $click = 1;
    }
    my $response = $self->execute("swipeWhileNotFound", RPC::XML::string->new($direction), RPC::XML::int->new($offset), RPC::XML::int->new($swipeTime), RPC::XML::string->new($zone), RPC::XML::string->new($elementtofind), RPC::XML::int->new($elementtofindindex), RPC::XML::int->new($delay), RPC::XML::int->new($rounds), RPC::XML::boolean->new($click));
    return $response->{'found'};
}

sub switchToBrowserTab() {
    my ($self, $tabId) = @_;
    my $response = $self->execute("switchToBrowserTab", RPC::XML::string->new($tabId));
}

sub sync() {
    my ($self, $silentTime, $sensitivity, $timeout) = @_;
    if (not defined ($silentTime)) {
        $silentTime = 2000;
    }
    if (not defined ($sensitivity)) {
        $sensitivity = 0;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("sync", RPC::XML::int->new($silentTime), RPC::XML::int->new($sensitivity), RPC::XML::int->new($timeout));
    return $response->{'found'};
}

sub syncElements() {
    my ($self, $silentTime, $timeout) = @_;
    if (not defined ($silentTime)) {
        $silentTime = 2000;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("syncElements", RPC::XML::int->new($silentTime), RPC::XML::int->new($timeout));
    return $response->{'found'};
}

sub textFilter() {
    my ($self, $color, $sensitivity) = @_;
    if (not defined ($sensitivity)) {
        $sensitivity = 15;
    }
    my $response = $self->execute("textFilter", RPC::XML::string->new($color), RPC::XML::int->new($sensitivity));
}

sub touchDown() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("touchDown", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
}

sub touchDownCoordinate() {
    my ($self, $x, $y) = @_;
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("touchDownCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y));
}

sub touchMove() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("touchMove", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
}

sub touchMoveCoordinate() {
    my ($self, $x, $y) = @_;
    if (not defined ($x)) {
        $x = 0;
    }
    if (not defined ($y)) {
        $y = 0;
    }
    my $response = $self->execute("touchMoveCoordinate", RPC::XML::int->new($x), RPC::XML::int->new($y));
}

sub touchUp() {
    my ($self) = @_;
    my $response = $self->execute("touchUp");
}

sub uninstall() {
    my ($self, $application) = @_;
    my $response = $self->execute("uninstall", RPC::XML::string->new($application));
    return $response->{'found'};
}

sub verifyElementFound() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("verifyElementFound", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
    if ($self->isDieOnFail()) {
        if (defined $response->{'errorMassage'}) {
            die $response->{'errorMassage'};
        }
    }
}

sub verifyElementNotFound() {
    my ($self, $zone, $element, $index) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    my $response = $self->execute("verifyElementNotFound", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index));
    if ($self->isDieOnFail()) {
        if (defined $response->{'errorMassage'}) {
            die $response->{'errorMassage'};
        }
    }
}

sub verifyIn() {
    my ($self, $zone, $searchElement, $index, $direction, $elementFindZone, $elementToFind, $width, $height) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($width)) {
        $width = 0;
    }
    if (not defined ($height)) {
        $height = 0;
    }
    my $response = $self->execute("verifyIn", RPC::XML::string->new($zone), RPC::XML::string->new($searchElement), RPC::XML::int->new($index), RPC::XML::string->new($direction), RPC::XML::string->new($elementFindZone), RPC::XML::string->new($elementToFind), RPC::XML::int->new($width), RPC::XML::int->new($height));
    if ($self->isDieOnFail()) {
        if (defined $response->{'errorMassage'}) {
            die $response->{'errorMassage'};
        }
    }
}

sub waitForAudioPlayEnd() {
    my ($self, $timeout) = @_;
    my $response = $self->execute("waitForAudioPlayEnd", RPC::XML::int->new($timeout));
}

sub waitForElement() {
    my ($self, $zone, $element, $index, $timeout) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("waitForElement", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($timeout));
    return $response->{'found'};
}

sub waitForElementToVanish() {
    my ($self, $zone, $element, $index, $timeout) = @_;
    if (not defined ($index)) {
        $index = 0;
    }
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("waitForElementToVanish", RPC::XML::string->new($zone), RPC::XML::string->new($element), RPC::XML::int->new($index), RPC::XML::int->new($timeout));
    return $response->{'found'};
}

sub waitForWindow() {
    my ($self, $name, $timeout) = @_;
    if (not defined ($timeout)) {
        $timeout = 10000;
    }
    my $response = $self->execute("waitForWindow", RPC::XML::string->new($name), RPC::XML::int->new($timeout));
    return $response->{'found'};
}
