var UPDATE_WAIT_TIME = 200;

function setAPIValueFromSlider(id, value) {
	if (id == "brightness-slider") {
		setLPBrightness(value);
	} else if (id == "gamma-slider") {
		//setLPGamma() TODO
	} else if (id == "smooth-slider") {
		setLPSmooth(value);
	}
}

// Init sliders
$("div.slider").slider({
	min: 0,
	max: 100,
	slide: function(e, ui) {
		setAPIValueFromSlider(this.id, ui.value);
	}
});

function setBrightnessSlider(val) {
	$("#brightness-slider").slider("value", val);
}

function setGammaSlider(val) {
	$("#gamma-slider").slider("value", val);
}

function setSmoothSlider(val) {
	$("#smooth-slider").slider("value", val);
}

// Toggle button
$("#turn-off-on").click(function(){
    var nextState = $(this).attr("data-alternate");
    var curText = $(this).text();
    $(this).text(nextState);
    $(this).attr("data-alternate", curText);
});

$("nav ul").on("click", "li", function(){
    $("#content div.page").removeClass("open");
    var name = this.id.substring(this.id.indexOf("-"));
    $("#page" + name).addClass("open");
});