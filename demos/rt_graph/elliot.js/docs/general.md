# General documentation for {elliot.js}

{elliot.js} is a near realtime graphing library written in JavaScript. It has no external dependencies.

## Usage

Here is a simple HTML page that illustates the use of {elliot.js}. This example assumes that you have `elliot.js` in a folder called `js`.

	<html>
		<head>
			<title>Elliot.js test</title>
			<link href='http://fonts.googleapis.com/css?family=Nobile:400,700' rel='stylesheet' type='text/css'>
			<script type='text/javascript' src='js/elliot.js'></script>
		</head>
		<body>
			<canvas id='myCanvas' width='600' height='200'>
				This text is shown to browsers lacking HTML 5 support
			</canvas>
		  	<script type="text/javascript">
		  		var elliot = new Elliot('myCanvas', {
					'general': {
						'background': '#2F2C2B', // Graph background color. Set to 'transparent' for transparency.
						'title': 'Cars inside the city gates', // Graph title
						'titleFont': 'Nobile', // Font name for the title
						'titleFontSize': 11, // Graph title font size
						'titleFontColor': '#ffffff', // Graph title font color
						'yAxisTitle': 'Cars', // Y axis title
						'yAxisFont': 'Nobile', // Font name for the Y axis
						'yAxisFontSize': 10, // Y axis font size
						'yAxisFontColor': '#ffffff', // Y axis font color
						'yAxisNumTicks': 7, // How many ticks should be shown
						'yAxisTickFontSize': 8, // Y axis tick font size
						'logLevel': 'INFO', // Set log level (DEBUG, INFO, WARNING, ERROR). Defaults to no logging.
					},
					'barGraph': {
						'markerPosition': 20, // Show marker every n bars. Set to 0 to turn markers off
						'updateFrequency': 500, // How often should the graph be updated, in ms
						'incrementalValues': true, // Should values be incremental?
						'barBackgroundColor': '#2F2C2B', // Color of the bars (where there is no data). Set to 'transparent' for transparency.
						'barColor': '#ff6060', // Color of data in the bars
						'markerColor': '#333333', // Color of the marker bars
						'barWidth': 3, // How many px should the bars be
						'barSpacing': 1, // How many px space should be between the bars
						'stickyScaling': false // Scale graph using data for the whole session (rather than currently displayed data)
					}
					});

				// Add some sample data
				elliot.add(Math.floor(2500000));
				var dataInterval2 = setInterval(function () {
					elliot.add(Math.floor((Math.random()*2200)+1));
					elliot.remove(Math.floor((Math.random()*1500)+1));
				}, 1000);
		  	</script>
	  	</body>
    </html>

## Function documentation

### init - Instanciate

You instanciate {elliot.js} by calling `Elliot()`. It takes two arguments

	Elliot(cavasID, configurationObject)

For example

	var elliot = new Elliot('myCanvas', config);

### add() - Add data

You add data to {elliot.js} by calling the `add(integer)` method:

	var elliot = new Elliot('myCanvas', config);
	elliot.add(10);

This function can be called wheter or not you have the `incrementalValues` set to `true`.

### remove() - Remove data

You remove data from {elliot.js} by calling the `remove(integer)` method:

	var elliot = new Elliot('myCanvas', config);
	elliot.remove(10);

This function can be called wheter or not you have the `incrementalValues` set to `true`, but it probably more important when you have it set to `true` as you might need to remove data from the value that {elliot.js} holds.

## Configuration parameters

Configuration to {elliot.js} is sent as a dictionary such as:

	var elliot = new Elliot('myCanvas', {
	    'general': {
			'background': '#2F2C2B', // Graph background color. Set to 'transparent' for transparency.
			'title': 'Cars inside the city gates', // Graph title
			'titleFont': 'Nobile', // Font name for the title
			'titleFontSize': 11, // Graph title font size
			'titleFontColor': '#ffffff', // Graph title font color
			'yAxisTitle': 'Cars', // Y axis title
			'yAxisFont': 'Nobile', // Font name for the Y axis
			'yAxisFontSize': 10, // Y axis font size
			'yAxisFontColor': '#ffffff', // Y axis font color
			'yAxisNumTicks': 7, // How many ticks should be shown
			'yAxisTickFontSize': 8, // Y axis tick font size
			'logLevel': 'INFO', // Set log level (DEBUG, INFO, WARNING, ERROR). Defaults to no logging.
	    },
	    'barGraph': {
			'markerPosition': 20, // Show marker every n bars. Set to 0 to turn markers off
			'updateFrequency': 500, // How often should the graph be updated, in ms
			'incrementalValues': true, // Should values be incremental?
			'barBackgroundColor': '#2F2C2B', // Color of the bars (where there is no data). Set to 'transparent' for transparency.
			'barColor': '#ff6060', // Color of data in the bars
			'markerColor': '#333333', // Color of the marker bars
			'barWidth': 3, // How many px should the bars be
			'barSpacing': 1, // How many px space should be between the bars
			'stickyScaling': false // Scale graph using data for the whole session (rather than currently displayed data)
	    }
    });

As you can see the configuration is separated into two segments, 'general' and 'barGraph'. The naming is probably self-explaining. Settings under 'general' is not specific to the type of graph generated, while settings under 'barGraph' are specific for Elliot.BarGraph's.

### General configuration

Configuration parameters for the 'general' section.

<table>
	<tr>
		<th>Parameter</th>
		<th>Expected value</th>
		<th>Comment</th>
	</tr>
	<tr>
		<td>background</td>
		<td>Hex color</td>
		<td>Graph background color. Set to 'transparent' for transparency.</td>
	</tr>
	<tr>
		<td>title</td>
		<td>String</td>
		<td>Graph title</td>
	</tr>
	<tr>
		<td>titleFont</td>
		<td>String</td>
		<td>Graph title font name</td>
	</tr>
	<tr>
		<td>titleFontSize</td>
		<td>Integer</td>
		<td>Graph title font size, in pixels</td>
	</tr>
	<tr>
		<td>titleFontColor</td>
		<td>Hex color</td>
		<td>Graph title font color</td>
	</tr>
	<tr>
		<td>yAxisTitle</td>
		<td>String</td>
		<td>Title for the Y axis values</td>
	</tr>
	<tr>
		<td>yAxisFont</td>
		<td>String</td>
		<td>Font name for the Y axis title</td>
	</tr>
	<tr>
		<td>yAxisFontSize</td>
		<td>Integer</td>
		<td>Font size for the Y axis title</td>
	</tr>
	<tr>
		<td>yAxisFontColor</td>
		<td>Hex color</td>
		<td>Font color for the Y axis title</td>
	</tr>
	<tr>
		<td>yAxisNumTicks</td>
		<td>Integer</td>
		<td>Number of ticks (value points) on the Y axis</td>
	</tr>
	<tr>
		<td>yAxisTickFontSize</td>
		<td>Integer</td>
		<td>Font size for the Y axis ticks</td>
	</tr>
	<tr>
		<td>logLevel</td>
		<td>String or empty</td>
		<td>Set log level (DEBUG, INFO, WARNING, ERROR). Defaults to no logging.</td>
	</tr>
</table>

### Bar graph configuration


Configuration parameters for the 'barGraph' section.

<table>
	<tr>
		<th>Parameter</th>
		<th>Expected value</th>
		<th>Comment</th>
	</tr>
	<tr>
		<td>markerPosition</td>
		<td>Integer</td>
		<td>Show marker every n bars. Set to 0 to turn markers off</td>
	</tr>
	<tr>
		<td>updateFrequency</td>
		<td>Integer</td>
		<td>How often should the graph be updated, in milliseconds</td>
	</tr>
	<tr>
		<td>incrementalValues</td>
		<td>Boolean</td>
		<td>Should {elliot.js} save the current value, so that you only send deltas of the data changes continuously?</td>
	</tr>
	<tr>
		<td>barBackgroundColor</td>
		<td>Hex color</td>
		<td>Background of data bars where there is no data to show. Set this to 'transparent' to get transparency</td>
	</tr>
	<tr>
		<td>barColor</td>
		<td>Hex color</td>
		<td>Color of the data bars</td>
	</tr>
	<tr>
		<td>markerColor</td>
		<td>Hex color</td>
		<td>Color of the markers</td>
	</tr>
	<tr>
		<td>barWidth</td>
		<td>Integer</td>
		<td>Width of the data bars</td>
	</tr>
	<tr>
		<td>barSpacing</td>
		<td>Integer</td>
		<td>Spacing beteween the data bars</td>
	</tr>
	<tr>
		<td>stickyScaling</td>
		<td>Boolean</td>
		<td>Scale graph using data for the whole session (rather than currently displayed data)</td>
	</tr>
</table>

## Questions?

Please do not hesitate to contact me at sebastian.dahlgren@gmail.com or Twitter @sebdah.
