/*
*
* elliot.js v0.2.0
*
* AUTHOR
* Sebastian Dahlgren <sebastian.dahlgren@gmail.com>
*
* WEB PAGE
* https://github.com/sebdah/elliot.js
* http://www.sebastiandahlgren.se
*
* LICENSE
* Apache License version 2.0
* http://www.apache.org/licenses/LICENSE-2.0.html
*/


/* Simple JavaScript Inheritance
 * By John Resig http://ejohn.org/
 * MIT Licensed.
 */
(function(){
  var initializing = false, fnTest = /xyz/.test(function(){xyz;}) ? /\b_super\b/ : /.*/;

  // The base Class implementation (does nothing)
  this.Class = function(){};
 
  // Create a new Class that inherits from this class
  Class.extend = function(prop) {
    var _super = this.prototype;
   
    // Instantiate a base class (but only create the instance,
    // don't run the init constructor)
    initializing = true;
    var prototype = new this();
    initializing = false;
   
    // Copy the properties over onto the new prototype
    for (var name in prop) {
      // Check if we're overwriting an existing function
      prototype[name] = typeof prop[name] == "function" &&
        typeof _super[name] == "function" && fnTest.test(prop[name]) ?
        (function(name, fn){
          return function() {
            var tmp = this._super;
           
            // Add a new ._super() method that is the same method
            // but on the super-class
            this._super = _super[name];
           
            // The method only need to be bound temporarily, so we
            // remove it when we're done executing
            var ret = fn.apply(this, arguments);
            this._super = tmp;
           
            return ret;
          };
        })(name, prop[name]):
        prop[name];
    }
   
    // The dummy class constructor
    function Class() {
      // All construction is actually done in the init method
      if ( !initializing && this.init )
        this.init.apply(this, arguments);
    }
   
    // Populate our constructed prototype object
    Class.prototype = prototype;
   
    // Enforce the constructor to be what we expect
    Class.prototype.constructor = Class;

    // And make this class extendable
    Class.extend = arguments.callee;
   
    return Class;
  };
})("Class");

/*
 * Elliot graphing library implementation
 *
 */
var Elliot = Class.extend({
	init: function (canvas_id, config) {
		// Canvas and context
		this.canvas = document.getElementById(canvas_id);
		this.context = this.canvas.getContext('2d');

		// Title bar height
		var titleBarHeight = 20;

		// Config object
		this.config = config;

		// Graph config object
		this.graph = {};

		// Set configuration defaults
		if (typeof(this.config['general']['background']) === 'undefined') { this.config['general']['background'] = '#ffffff'; }
		if (typeof(this.config['general']['title']) === 'undefined') { this.config['general']['title'] = ''; }
		if (typeof(this.config['general']['titleFont']) === 'undefined') { this.config['general']['titleFont'] = 'Arial'; }
		if (typeof(this.config['general']['titleFontSize']) === 'undefined') { this.config['general']['titleFontSize'] = 11; }
		if (typeof(this.config['general']['titleFontColor']) === 'undefined') { this.config['general']['titleFontColor'] = '#000000'; }
		if (typeof(this.config['general']['yAxisTitle']) === 'undefined') { this.config['general']['yAxisTitle'] = ''; }
		if (typeof(this.config['general']['yAxisFont']) === 'undefined') { this.config['general']['yAxisFont'] = 'Arial'; }
		if (typeof(this.config['general']['yAxisFontSize']) === 'undefined') { this.config['general']['yAxisFontSize'] = 10; }
		if (typeof(this.config['general']['yAxisFontSize']) === 'undefined') { this.config['general']['yAxisFontSize'] = 10; }
		if (typeof(this.config['general']['yAxisFontColor']) === 'undefined') { this.config['general']['yAxisFontColor'] = '#000000'; }
		if (typeof(this.config['general']['yAxisNumTicks']) === 'undefined') { this.config['general']['yAxisNumTicks'] = 2; }
		if (typeof(this.config['general']['yAxisTickFontSize']) === 'undefined') { this.config['general']['yAxisTickFontSize'] = 8; }
		if (typeof(this.config['general']['logLevel']) === 'undefined') { this.config['general']['logLevel'] = ''; }

		// Set log level
		this.logLevel = 0;
		if (this.config['general']['logLevel'] === 'DEBUG') { this.logLevel = 4; }
		if (this.config['general']['logLevel'] === 'INFO') { this.logLevel = 3; }
		if (this.config['general']['logLevel'] === 'WARNING') { this.logLevel = 2; }
		if (this.config['general']['logLevel'] === 'ERROR') { this.logLevel = 1; }

		// Check that the user has defined width and height on the canvas
		if (isNaN(this.canvas.width) || isNaN(this.canvas.height)) {
			this.logError('You must set both width and height on your canvas');
		} else {
			// Height and width
			this.graph.width = this.canvas.width - 120;
			this.graph.height = this.canvas.height - titleBarHeight;
			this.graph.x = 0;
			this.graph.y = titleBarHeight;

			// Scaling setting
			this.graph.scale = 1;

			this.logInfo('Canvas dimensions set to ' + this.canvas.width + 'x' + this.canvas.height);
			this.logInfo('Graph dimensions set to ' + this.graph.width + 'x' + this.graph.height);
			this.logInfo('Graph coordinates (' + this.graph.x + ',' + this.graph.y + ')');
		}
	},
	logDebug: function (message) { if (this.logLevel >= 4) { console.log(this.canvas.id + ' - DEBUG - ' + message); } },
	logInfo: function (message) { if (this.logLevel >= 3) { console.log(this.canvas.id + ' - INFO - ' + message); } },
	logWarning: function (message) { if (this.logLevel >= 2) { console.log(this.canvas.id + ' - WARNING - ' + message); } },
	logError: function (message) { if (this.logLevel >= 1) { console.log(this.canvas.id + ' - ERROR - ' + message); } },
	drawBackground: function () {
		this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
		
		if (this.config['general']['background'] !== 'transparent') {
			this.context.fillStyle = this.config['general']['background'];
			this.context.fillRect(0, 0, this.canvas.width, this.canvas.height);
		}
	}
});

var ElliotBarGraph = Elliot.extend({
	init: function (canvas_id, config) {
		// Add config to the super class
		this._super(canvas_id, config);

		// Set config option default
		if (typeof(this.config['barGraph']['markerPosition']) === 'undefined') { this.config['barGraph']['markerPosition'] = 10; }
		if (typeof(this.config['barGraph']['updateFrequency']) === 'undefined') { this.config['barGraph']['updateFrequency'] = 500; }
		if (typeof(this.config['barGraph']['incrementalValues']) === 'undefined') { this.config['barGraph']['incrementalValues'] = false; }
		if (typeof(this.config['barGraph']['barBackgroundColor']) === 'undefined') { this.config['barGraph']['barBackgroundColor'] = '#ffffff'; }
		if (typeof(this.config['barGraph']['barColor']) === 'undefined') { this.config['barGraph']['barColor'] = '#000000'; }
		if (typeof(this.config['barGraph']['markerColor']) === 'undefined') { this.config['barGraph']['markerColor'] = '#777777'; }
		if (typeof(this.config['barGraph']['barWidth']) === 'undefined') { this.config['barGraph']['barWidth'] = 5; }
		if (typeof(this.config['barGraph']['barSpacing']) === 'undefined') { this.config['barGraph']['barSpacing'] = 5; }
		if (typeof(this.config['barGraph']['stickyScaling']) === 'undefined') { this.config['barGraph']['stickyScaling'] = false; }

		// Updated barData
		this.updatedBarData = [];
		this.nextValue = 0;

		// Lowest min value (for the whole session)
		this.minValue = NaN;
		this.maxValue = NaN;

		// Distinguish the first iteration
		this.first = true;

		// Incremental values
		if (typeof(this.config['barGraph']['incrementalValues']) === 'undefined') {
			this.incrementalValues = false;
		} else {
			this.incrementalValues = this.config['barGraph']['incrementalValues'];
		}

		// Offset for bar marker counting
		this.offset = 0;

		// Update frequency
		if (typeof(this.config['barGraph']['updateFrequency']) === 'undefined') {
			this.updateFrequency = 500;
		} else {
			this.updateFrequency = this.config['barGraph']['updateFrequency'];
		}

		// Update the graph continously
		this.drawBarGraph();
		this.drawInterval = setInterval((function (self) {
			return function () {
				self.drawBarGraph();
			};
		})(this), this.updateFrequency);
	},
	add: function (count) {
		if (typeof(count) === 'undefined') {
			this.nextValue += 1;
		} else {
			this.nextValue += count;
		}
	},
	drawBarGraph: function () {
		/*
		* Fill the background
		*/
		this.context.save();
		this.drawBackground();
		this.context.restore();

		/*
		* Graph title
		*/
		this.context.save();
		var titleMetrics = this.context.measureText(this.config['general']['title']); // Measure the size of the text object
		this.context.font = 'bold ' + this.config['general']['titleFontSize'] + 'pt ' + this.config['general']['titleFont'];
		this.context.fillStyle = this.config['general']['titleFontColor'];
		this.context.fillText(
			this.config['general']['title'],	// Text
			(this.graph.width - titleMetrics.width) / 2,	// x
			this.config['general']['titleFontSize'] + 3);	// y
		this.context.restore();

		/*
		* Y axis title
		*/
		this.context.save();
		var yAxisTitleMetrics = this.context.measureText(this.config['general']['yAxisTitle']);
		this.context.font = 'bold ' + this.config['general']['yAxisFontSize'] + 'pt ' + this.config['general']['yAxisFont'];
		this.context.fillStyle = this.config['general']['yAxisFontColor'];
		this.context.fillText(
			this.config['general']['yAxisTitle'],
			this.canvas.width - yAxisTitleMetrics.width * 1.8,
			this.canvas.height - (this.graph.height / 2));
		this.context.restore();

		
		// Calculate how many bars we have
		var numBars = this.graph.width / (this.config['barGraph']['barSpacing'] + this.config['barGraph']['barWidth']);

		// Add the last data point to the data list
		if (!this.first) {
			this.updatedBarData.splice(0, 1); // Remove first item in array
			this.updatedBarData.push(this.nextValue); // Add next value to array
			this.logDebug('Added value to graph: ' + this.nextValue);

			// Reset the nextValue if we are not counting incrementally
			if (!this.incrementalValues) {
				this.nextValue = 0;
			}
		} else {
			this.first = false;
		}

		/*
		* Calculate min and max value
		*/
		var minValue = NaN;
		var onlyZerosSoFar = true; // Used to keep track of initial zeros in array
		for (i = 0; i < this.updatedBarData.length; i++) {
			// Keep track of all inital zeros
			if (onlyZerosSoFar && this.updatedBarData[i] !== 0) {
				onlyZerosSoFar = false;
			}

			// Add the value if it is applicable
			if (!onlyZerosSoFar) {
				if (isNaN(minValue)) {
					minValue = this.updatedBarData[i];
				} else if (minValue > this.updatedBarData[i]) {
					minValue = this.updatedBarData[i];
				}
			}
		}

		// Store it globally
		if (isNaN(this.minValue) || minValue < this.minValue) {
			this.minValue = minValue;
		}

		// Use the min value if requested
		if (this.config['barGraph']['stickyScaling']) {
			minValue = this.minValue;
		}

		// Max value
		maxValue = Math.max.apply(Math, this.updatedBarData);

		// Store it globally
		if (isNaN(this.maxValue) || maxValue > this.maxValue) {
			this.maxValue = maxValue;
		}

		// Use the min value if requested
		if (this.config['barGraph']['stickyScaling']) {
			maxValue = this.maxValue;
		}

		// Update the incoming data to match the graph
		if (this.updatedBarData.length > numBars + 1) {
			while (this.updatedBarData.length > numBars) { // Remove data until the array has a proper length
				this.updatedBarData.splice(0, 1);
			}
		} else if (this.updatedBarData.length <= numBars) { // Add zeros until the array has a proper length
			while (this.updatedBarData.length <= numBars) {
				this.updatedBarData.push(0);  // Add 0 to the data
			}
		}

		/*
		* Scaling logic
		*/
		// The 0.9 indicates that we are scaling when the data reaches 90% of the graph
		if (this.graph.scale != Math.round((maxValue / (this.graph.height * 0.9)) + 0.6)) {
			this.graph.scale = Math.round((maxValue / (this.graph.height * 0.9)) + 0.6);
			if (this.graph.scale < 1) {
				this.graph.scale = 1;
			}
			this.logDebug("Scale changed to " + this.graph.scale);
		}
		if (this.graph.minValue < 0) {
			minValue = 0;
			this.minValue = 0;
		}
		this.graph.scaledHeight = this.graph.height * this.graph.scale;
		this.graph.maxValue = maxValue + ((maxValue - minValue) * 0.2);
		this.graph.minValue = minValue - ((maxValue - minValue) * 0.2);
		if (this.graph.minValue < 0 ) {
			this.graph.minValue = 0;
		}
		// Add some padding if the minValue is zero
		if (this.graph.minValue === 0){
			this.graph.maxValue = maxValue * 1.1;
		}

		/*
		* Add Y axis ticks
		*/
		this.context.save();
		this.context.font = 'bold ' + this.config['general']['yAxisTickFontSize'] + 'pt ' + this.config['general']['yAxisFont'];
		this.context.fillStyle = this.config['general']['yAxisFontColor'];
		for (i = 0; i <= this.config['general']['yAxisNumTicks']; i++) {
			if (i === 0) {
				this.context.fillText(
					Math.round(this.graph.minValue), // Tick text
					this.graph.width + 3, // x
					this.canvas.height - 2 - ((this.graph.height) / this.config['general']['yAxisNumTicks']) * i); // y
			} else {
				this.context.fillText(
					Math.round((((this.graph.maxValue - this.graph.minValue) / this.config['general']['yAxisNumTicks']) * i) + this.graph.minValue), // Tick text
					this.graph.width + 3, // x
					this.canvas.height - ((this.graph.height) / this.config['general']['yAxisNumTicks']) * i); // y
			}
		}
		this.context.restore();

		/*
		* Draw all bars
		*/
		this.context.save();
		var currentBar = numBars;
		i = 0;
		for (var x = this.graph.x; x < this.graph.width - this.config['barGraph']['barSpacing'] - this.config['barGraph']['barWidth']; x += this.config['barGraph']['barSpacing'] + this.config['barGraph']['barWidth']) {
			// We do not handle negative numbers
			var val = 0;
			if (this.updatedBarData[i] < 0) {
				this.updatedBarData[i] = 0;
			} else {
				val = this.updatedBarData[i];
			}

			// Background bar
			if (currentBar % this.config['barGraph']['markerPosition'] - this.offset === 0) {
				this.context.fillStyle = this.config['barGraph']['markerColor'];
			} else {
				this.context.fillStyle = this.config['barGraph']['barBackgroundColor'];
			}
			this.context.fillRect(
				x + this.config['barGraph']['barSpacing'],
				this.graph.y,
				this.config['barGraph']['barWidth'],
				this.graph.height);

			// Add bottom line (the bottom 1 pixel)
			if (this.config['barGraph']['barColor'] !== 'transparent') {
				this.context.fillStyle = this.config['barGraph']['barColor'];
			}
			this.context.fillRect(
				x + this.config['barGraph']['barSpacing'],
				this.graph.y + this.graph.height - 1,
				this.config['barGraph']['barWidth'],
				this.graph.height);

			// Add data rect
			var point = this.graph.height - ((this.graph.height / (this.graph.maxValue - this.graph.minValue)) * (this.graph.maxValue - this.updatedBarData[i]));
			this.context.fillStyle = this.config['barGraph']['barColor'];
			this.context.fillRect(
				x + this.config['barGraph']['barSpacing'],
				this.graph.y + this.graph.height - point,
				this.config['barGraph']['barWidth'],
				this.graph.height);

			// Back one bar every time
			currentBar--;
			i++;
		}
		this.context.restore();

		// Update the offset
		if (this.offset < this.config['barGraph']['markerPosition'] - 1) {
			this.offset++;
		} else {
			this.offset = 0;
		}
	},
	remove: function (count) {
		if (typeof(count) === 'undefined') {
			this.nextValue -= 1;
		} else {
			this.nextValue -= count;
		}
	}
});
