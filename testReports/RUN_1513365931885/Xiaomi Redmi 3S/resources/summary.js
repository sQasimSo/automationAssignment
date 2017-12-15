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

/** Script File for innerNavigation of Reporting **/
function setLocation(newURL, isIFrame) {
	if (isIFrame) {
		$("#main-menu").fadeOut(100);
		
		try{
			parent.frames['report-iframe'].contentWindow.location.replace(newURL);
		}
		catch(error){			
			if(error.name == 'NS_ERROR_UNKNOWN_PROTOCOL' && window.location.protocol == 'file:'){ // firefox bug
				var fireURL = window.location.protocol + "//" + newURL;
				parent.frames['report-iframe'].contentWindow.location.replace(fireURL);
			}
			else{ // safari bug
				parent.frames['report-iframe'].location.replace(newURL);
			}
		}
		
		$('#main-report').fadeIn(500);
		$('iframe').focus();
	} else {
		window.location.assign(newURL);
	}
}

function RedirectOnLoad() {
	if (window.location.hash != '') {
		var hash = window.location.hash;
		var parseHash = hash.replace('#', '');

		var query = "td span[href~=\"" + parseHash + "\"]";
		$(query).parent().parent().attr('class', 'current');

		document.title = $(query).text();

		var iframeurl = parseHash;
		
        var fileElement = document.querySelector(query);
		var fileName = "index.html";
		
		try{
			fileName = fileElement.getAttribute("file");
		}catch(err){
			
		}
		
		if (iframeurl.indexOf(fileName) == -1) {
			iframeurl = iframeurl + "/" + fileName;
		}
		

		setLocation(iframeurl, !isIE());
	} else {
		$("#main-menu").fadeIn(100);
		$("#main-report").fadeOut(100);
	}
};

window.onload = function (e) {

	RedirectOnLoad();

	$(window).bind('hashchange', function () {
		RedirectOnLoad()
	});

	// search field
	var searchElement = document.getElementById('filterField');

	if (searchElement != null) {
		searchElement.onkeyup = function () {
			var newText = $(this).val();
			if (newText == "") {
				$('tr').show();
				$('.highlighted').removeClass('highlighted');
			} else {
				$('tbody tr').each(function () {

					if(this == $('#noresultrow')[0]){
					}
					else if ($(this).text().toLowerCase().indexOf(newText.toLowerCase()) < 0) {
						$(this).hide();
					} else {
						$(this).show();

						$(this.cells).each(function () {
							if ($(this).text().toLowerCase().indexOf(newText.toLowerCase()) < 0) {
								$(this).children().removeClass('highlighted');
							} else {
								$(this).children().addClass('highlighted');
							}
						});
					}
				});
			}
			if ($('tbody tr:not(#noresultrow):visible').length == 0) {
				$('#noresultrow').show();
				$('#noresultrow-searchtext').text(newText);
			} else {
				$('#noresultrow').hide();
			}
		};
	};

	$("#main-menu tbody tr").click(function () {

		var url = $(this.cells[1]).find("span").attr("href");
		var pUrl = $(this.cells[1]).find("span").attr("href").replace("/index.html", "").replace("\\index.html", "");

		if (pUrl == '' || pUrl == undefined) {}
		else {
			document.title = this.cells[1].innerText;
			$("#main-menu").fadeOut(100, function () {
				if (!isIE()) {
					window.location.hash = pUrl;
				} else {
					window.location.assign(url);
				}
			});
		}
	});

	$("#returntomenu").click(function () {
		var nURL = "index.html";
		$("#main-report").fadeOut(100, function () {
			window.location.hash = "";
		});
		document.title = "Summary Report";
	});

	$("#nextbutton").click(function () {
		var nURL = $("#main-menu tbody .current").next();

		if (nURL != '' && nURL != undefined && nURL.find('span').length > 0) {
			$("#main-report").fadeOut(100, function () {
				$("#main-menu tbody .current").next().click();
				$('.current').removeClass();
			});
		}
	});

	$("#backbutton").click(function () {
		var nURL = $("#main-menu tbody .current").prev();

		if (nURL != '' && nURL != undefined && nURL.find('span').length > 0) {
			$("#main-report").fadeOut(100, function () {
				$("#main-menu tbody .current").prev().click();
				$('.current').removeClass();
			});
		}
	});
};
