/** Script File for Reporting **/
function isIE(){
	
	if ((navigator.userAgent.toLowerCase().indexOf("chrome") !== -1) || (navigator.userAgent.toLowerCase().indexOf("safari") !== -1) || (navigator.userAgent.toLowerCase().indexOf("firefox") !== -1)){
		return false;
	}
	else{
		return true;
	}
}

if (typeof $ === "undefined") {
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

$(document).ready(function(){

	$('#report-bar-ul li').click(function(){
		$('#report-bar-ul li').removeClass('selected');
		$(this).addClass('selected');

		$('.results').hide();
		$('.results .sub-results').hide();
		$('.results').find('.sub-results:first').show();
		
		var resultDiv = $('.results:eq('+$(this).attr('data-step-counter')+')');
		$(resultDiv).find('.selected').click();
		$(resultDiv).show();

		$(resultDiv).find('ul.sub-tabs li').click(function(){
			$(resultDiv).find('ul.sub-tabs li').removeClass('selected');
			$(this).addClass('selected');
			$(resultDiv).find(' .sub-results').hide();
			$(resultDiv).find(' .sub-results:eq('+$(this).index()+')').show();
		});

		// scroll to element if needed
		try{
			this.scrollIntoViewIfNeeded();
		}
		catch(err){
			this.scrollIntoView();
		}
	});
	
	// popover on mmouse hover over left panel items
	$('.left-panel .list-group-item[data-content]').popover({ html: true , container: 'body', trigger: 'hover', delay: { "show": 500, "hide": 0 } });

	// custom height for ie
	if (isIE()){
		$(window).resize(function() {setElementsHeight()});
		setElementsHeight();
	}
	
	// handle keyboard shortcuts
	document.onkeydown = function(evt) {

		evt = evt || window.event;
		switch (evt.keyCode) {
			case 37: // left
				myPreventDefault(evt);
				
				if (evt.shiftKey==1){
					window.parent.leftArrowAndShiftPressed();
				}
				else{
					leftArrowPressed();
				}
				
				break;

			case 38: // up
				myPreventDefault(evt);
				upArrowPressed();
				break;

			case 39: // right
				myPreventDefault(evt);
				
				if (evt.shiftKey==1){
					window.parent.rightArrowAndShiftPressed();
				}
				else{
					rightArrowPressed();
				}
				
				break;
			
			case 40: // down
				myPreventDefault(evt);
				downArrowPressed();
				break;
		}	
	};
	
});

// handle divider drag
$(document).ready(function(){

    $('#vertical-divider').on('mousedown', function(e){
	
		e.preventDefault();
		
        var $dragable = $(this),
            startWidth = $dragable.parent().width(),
            pX = e.pageX;
			
        $(document).on('mouseup', function(e){
            $(document).off('mouseup').off('mousemove');
        });
		
        $(document).on('mousemove', function(me){
			
			var mx = (me.pageX - pX);
			            
			var l = (((startWidth + mx) / window.innerWidth) * 100);
			var w = (((window.innerWidth - startWidth - mx) / window.innerWidth) * 100) + '%';
			
			if(l < 20 || l > 70){
				return;
			}
			
			l = l + '%';
			
            $dragable.css({
                left: l
            });
			
			$dragable.parent().css({
                width: l
            });
			
			$('.right-panel').css({
				left: l,
				width: w
			});
			
			$('.right-panel .results').css({
				width: w
			});
        });
                
    });
});

function setElementsHeight(){
		var totalHeight = $(window).height();
		
		var calcNewHeight = totalHeight - 60;
				
		if (totalHeight > 140)
		{
			$(".body-content .content").css("height", calcNewHeight + "px");
			$(".left-panel").css("height", calcNewHeight + "px");
		}
}

function upArrowPressed(){

	var element = $('.list-group-item.selected')[0];
	var next = parseInt($('.list-group-item:visible').index(element) - 1);
	var newElement = $('.list-group-item:visible')[next];
	
	if (newElement) {
		$(newElement).click();
		
		try{
			newElement.scrollIntoViewIfNeeded();
		}
		catch(err){
			newElement.scrollIntoView();
		}
	}
}

function downArrowPressed(){
	if (parseInt($('.left-panel .selected').attr('data-step-counter')) == 0){
		$('#report-bar-ul li:first').next().click();
	}
	else{
		var element = $('.list-group-item.selected')[0];
		var next = parseInt($('.list-group-item:visible').index(element) + 1);
		var newElement = $('.list-group-item:visible')[next];
		
		if (newElement) {
			$(newElement).click();
			
			try{
				newElement.scrollIntoViewIfNeeded();
			}
			catch(err){
				newElement.scrollIntoView();
			}
		}

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

$(document.body).ready(function(){
	//SA-6031:
   	//Assign the actual URL of the current page to the value of argument AppPropConstants.MAIN_PAGE_URL
   	//This value will be used for accessing scene files stored in the current report  
   	$(".dump-link").each(function() {
    	var link = $(this).attr('href');
       	link = link.replace(/m_A_i_N__p_A_g_E__u_R_l=/g, "m_A_i_N__p_A_g_E__u_R_l=" + encodeURIComponent(window.location.href.toString()));
       	//Perhaps, window.location.href should be replaced by document.URL in the line above.
       	$(this).attr('href', link);
   	});
});

function myPreventDefault(event){
	try{
		event.preventDefault();
	}
	catch(err){
		event.returnValue = false;
	}
}