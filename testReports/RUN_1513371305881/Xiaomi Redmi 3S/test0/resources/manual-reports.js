/** Script File for Reporting **/

function isIE(){
	
	if ((navigator.userAgent.toLowerCase().indexOf("chrome") !== -1) || (navigator.userAgent.toLowerCase().indexOf("safari") !== -1) || (navigator.userAgent.toLowerCase().indexOf("firefox") !== -1)){
		return false;
		}
	else{
		return true;
		}
}

if (typeof jQuery === "undefined") {
    var script = document.createElement('script');
    	
	if(isIE()){
	    script.src = 'resources/jquery-1.11.1.min.js';
	}
	else{
	    script.src = 'resources/jquery-2.1.0.min.js';		
	}

    script.type = 'text/javascript';
    document.getElementsByTagName('head')[0].appendChild(script);
}

window.onload = function(){
	jQuery('#report-bar-ul li').click(function(){
		jQuery('#report-bar-ul li').attr('class','');
		jQuery(this).attr('class','selected');
		jQuery('.results').hide();
		
		resultDiv = jQuery('.results:eq('+jQuery(this).index()+')');
		jQuery(resultDiv).show();
		jQuery(resultDiv).find('ul.sub-tabs li').click(function(){
			jQuery(resultDiv).find('ul.sub-tabs li').attr('class','');
			jQuery(this).attr('class','selected');
			jQuery(resultDiv).find(' .sub-results').hide();
			jQuery(resultDiv).find(' .sub-results:eq('+jQuery(this).index()+')').show();
		});
	});
	
	// if not summary page
	if ($('#body-summary').length == 0 && isIE()){
		$(window).resize(function() {setElementsHeight()});
		setElementsHeight();
	}
	
	if (($('#body-summary').length == 0)){
		document.onkeydown = function(evt) {
	
			evt = evt || window.event;
			switch (evt.keyCode) {
				case 37: // left
					evt.preventDefault();
					
					if (event.shiftKey==1){
						window.parent.leftArrowAndShiftPressed();
					}
					else{
						leftArrowPressed();
					}
					
					break;

				case 38: // up
					evt.preventDefault();
					upArrowPressed();
					break;

				case 39: // right
					evt.preventDefault();
					
					if (event.shiftKey==1){
						window.parent.rightArrowAndShiftPressed();
					}
					else{
						rightArrowPressed();
					}
					
					break;
				
				case 40: // down
					evt.preventDefault();
					downArrowPressed();
					break;
			}	
		}
	};
};

function setElementsHeight(){
		var totalHeight = $(window).height();
		
		var calcNewHeight = totalHeight - 140;
		var calcNewHeightRightNav = totalHeight - 153;
		
		if (totalHeight > 140)
		{
			$(".body-content .content").css("height", calcNewHeight + "px");
			$(".left-panel").css("height", calcNewHeightRightNav + "px");
		}
		
		
		/* try this instead of the media queries
		var leftPanelWidth = $(".left-panel").css("width").replace('px','');
		var calcNewWidth = leftPanelWidth - 40;
		
		$("#report-bar-ul li span.txt").css("width", calcNewwidth + "px");
		*/
}

function upArrowPressed(){
	$('.selected .nmbr').parent().prev().click();
}

function downArrowPressed(){
	if ($('.selected .nmbr').text() == ""){
		$('#report-bar-ul li:first').next().click()
	}
	else{
		$('.selected .nmbr').parent().next().click();
	}
}

function rightArrowPressed(){
	var a = $(".sub-tabs").filter(":visible").filter("ul").children().length;
	
	if (a != 0){
		$(".sub-tabs:visible .selected").next().click();
	}
}

function leftArrowPressed(){
	var a = $(".sub-tabs").filter(":visible").filter("ul").children().length;
	
	if (a != 0){
		$(".sub-tabs:visible .selected").prev().click();
	}
}

function rightArrowAndShiftPressed(){
	$("#nextbutton").click();
}

function leftArrowAndShiftPressed(){
	$("#backbutton").click();
}