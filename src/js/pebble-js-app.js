var dateorder = 0;
var weekday = 0;
var bigminutes = 0;
var showdate = 0;
var lang = 0;
var negative = 0;
var themecode;
var btalert = 1;


function logVariables() {
	console.log("Variables");

	console.log("	dateorder: " + dateorder);
	console.log("	weekday: " + weekday);
	console.log("	bigminutes: " + bigminutes);
	console.log("	showdate: " + showdate);
	console.log("	lang: " + lang);
	console.log("	negative: " + negative);
  console.log("	themecode: " + themecode);
  console.log("	btalert: " + btalert);
}


function isWatchColorCapable() {
  if (typeof Pebble.getActiveWatchInfo === "function") {
    try {
      if (Pebble.getActiveWatchInfo().platform != 'aplite') {
        return true;
      } else {
        return false;
      }
    } catch(err) {
      console.log('ERROR calling Pebble.getActiveWatchInfo() : ' + JSON.stringify(err));
      // Assuming Pebble App 3.0
      return true;
    }
  } else {
    return false;
  }
  //return ((typeof Pebble.getActiveWatchInfo === "function") && Pebble.getActiveWatchInfo().platform!='aplite');
}

Pebble.addEventListener("ready", function() {
	console.log("Ready Event");
	dateorder = localStorage.getItem("dateorder");
	if (!dateorder) {
		dateorder = 1;
	}
	
	weekday = localStorage.getItem("weekday");
	if (!weekday) {
		weekday = 0;
	}
	
	bigminutes = localStorage.getItem("bigminutes");
	if (!bigminutes) {
		bigminutes = 0;
	}
	
	showdate = localStorage.getItem("showdate");
	if (!showdate) {
		showdate = 1;
	}
	
  lang = localStorage.getItem("lang");
  if (!lang) {
		lang = 1;
  }

	negative = localStorage.getItem("negative");
  if (negative != 1) {
		negative = 0;
	}

  themecode = localStorage.getItem("themecode");
  if (!themecode || (""+themecode == "None")) {
    themecode = "c0fef8c0fd";
  }

	btalert = localStorage.getItem("btalert");
  if (btalert != 0) {
		btalert = 1;
	}


	logVariables();

  var msg = '{';
  msg += '"dateorder":'+dateorder;
  msg += ',"weekday":'+weekday;
  msg += ',"lang":'+lang;
  msg += ',"bigminutes":'+bigminutes;
  msg += ',"showdate":'+showdate;
  msg += ',"negative":'+negative;
  msg += ',"themecode":"'+themecode+'"';
  msg += ',"btalert":'+btalert;
  msg += '}';
  console.log("Sending message to watch :");
  console.log(" " + msg);
	Pebble.sendAppMessage(JSON.parse(msg));

});

Pebble.addEventListener("showConfiguration", function(e) {
	console.log("showConfiguration Event");

	logVariables();
						
	Pebble.openURL("http://www.famillemattern.com/jnm/pebble/Clipped/Clipped_3.2.html?dateorder=" + dateorder + "&weekday=" + weekday + "&lang=" + lang + "&bigminutes=" + bigminutes + "&showdate=" + showdate + "&negative=" + negative + "&btalert=" + btalert + "&themecode=" + themecode + "&colorCapable=" + isWatchColorCapable());
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("Configuration window closed");
	console.log(e.type);
  console.log("Response: " + decodeURIComponent(e.response));

	var configuration = JSON.parse(decodeURIComponent(e.response));
	Pebble.sendAppMessage(configuration);
	
	dateorder = configuration["dateorder"];
	localStorage.setItem("dateorder", dateorder);
	
	weekday = configuration["weekday"];
	localStorage.setItem("weekday", weekday);

	bigminutes = configuration["bigminutes"];
	localStorage.setItem("bigminutes", bigminutes);

	showdate = configuration["showdate"];
	localStorage.setItem("showdate", showdate);

	lang = configuration["lang"];
	localStorage.setItem("lang", lang);

	negative = configuration["negative"];
	localStorage.setItem("negative", negative);

  themecode = configuration["themecode"];
	localStorage.setItem("themecode", themecode);

  btalert = configuration["btalert"];
	localStorage.setItem("btalert", btalert);
});
