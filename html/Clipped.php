<!DOCTYPE html>
<html>
        <head>
                <title>Clipped</title>
                <meta charset='utf-8'>
                <meta name='viewport' content='width=device-width, initial-scale=1'>
                <link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
                <script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
                <script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
                <style>
                        .ui-header .ui-title { margin-left: 1em; margin-right: 1em; text-overflow: clip; }
                </style>
        </head>
		<body>
<div data-role="page" id="page1">
    <div data-theme="a" data-role="header" data-position="fixed">
        <h3>
            Clipped Configuration
        </h3>
        <div class="ui-grid-a">
            <div class="ui-block-a">
                <input id="cancel" type="submit" data-theme="c" data-icon="delete" data-iconpos="left"
                value="Cancel">
            </div>
            <div class="ui-block-b">
                <input id="save" type="submit" data-theme="b" data-icon="check" data-iconpos="right"
                value="Save">
            </div>
        </div>
    </div>
    <div data-role="content">

	<div data-role="fieldcontain">
            <label for="showdate">
                Show Date
			</label>
            <select name="showdate" id="showdate" data-theme="" data-role="slider">
<?php
	if (!isset($_GET['showdate'])) {
		$showdate = 1;
	} else {
		$showdate = $_GET['showdate'];
	}
	
	if ($showdate == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>

		<div id="dateorder" data-role="fieldcontain">
			<fieldset data-role="controlgroup" data-type="horizontal">
				<legend>Date format</legend>

<?php
	if (!isset($_GET['dateorder'])) {
		$dateorder = 1;
	} else {
		$dateorder = $_GET['dateorder'];
	}
	
	if ($dateorder == 0) {
		$s1 = " checked";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " checked";
	}
	
	echo '<input id="format1" name="dateorder" value="0" data-theme="" type="radio"' . $s1 . '><label for="format1">dd mm</label>';
	echo '<input id="format2" name="dateorder" value="1" data-theme="" type="radio"' . $s2 . '><label for="format2">mm dd</label>';
?>
			</fieldset>
		</div>

        <div data-role="fieldcontain">
            <label for="weekday">
                Show day of week
            </label>
            <select name="weekday" id="weekday" data-theme="" data-role="slider">
<?php
	if (!isset($_GET['weekday'])) {
		$weekday = 1;
	} else {
		$weekday = $_GET['weekday'];
	}
	
	if ($weekday == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>

        <div data-role="fieldcontain">
            <label for="bigminutes">
                Big Digits show Minutes
            </label>
            <select name="bigminutes" id="bigminutes" data-theme="" data-role="slider">
<?php
	if (!isset($_GET['bigminutes'])) {
		$bigminutes = 0;
	} else {
		$bigminutes = $_GET['bigminutes'];
	}
	
	if ($bigminutes == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>


		<div data-role="fieldcontain">
			<label for="lang">
				Language
			</label>
			<select id="lang" data-native-menu="true" name="lang">

<?php
	$langs = array(
		0 => 'Dutch',
		1 => 'English',
		2 => 'French',
		3 => 'German',
		5 => 'Portuguese',
		4 => 'Spanish'
	);
	
	if (!isset($_GET['lang'])) {
		$lang = 1;
	} else {
		$lang = $_GET['lang'];
	}
	
	foreach ($langs as $v => $n) {
		if ($lang == $v) {
			$s = " selected";
		} else {
			$s = "";
		}
		echo '<option value="' . $v . '" ' . $s . '>' . $n . '</option>';
	}
	?>
			</select>
		</div>

	</div>
</div>

    <script>
      function saveOptions() {
        var options = {
			'dateorder': parseInt($("input[name=dateorder]:checked").val(), 10),
			'weekday': parseInt($("#weekday").val(), 10),
			'bigminutes': parseInt($("#bigminutes").val(), 10),
			'showdate': parseInt($("#showdate").val(), 10),
			'lang': parseInt($("#lang").val(), 10),
        }
        return options;
      }

      $().ready(function() {
        $("#cancel").click(function() {
          console.log("Cancel");
          document.location = "pebblejs://close#";
        });

        $("#save").click(function() {
          console.log("Submit");
          
          var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(saveOptions()));
          console.log("Close: " + location);
          console.log(location);
          document.location = location;
        });

      });
    </script>
</body>
</html>
