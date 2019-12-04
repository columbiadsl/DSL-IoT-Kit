/*
  	statemachine.js

	Finite state machine implementation using classes for States and Transitions. 
	Includes functions for creating state machines defined in CSV files.

 	Copyright (c) 2019 Jeff Gregorio. All rights reserved.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

include("pathutil");

// ===========================================================================
//
// StateMachine
//
// Class modeling states, transitions, and conditional statements specified 
// in a pair of CSV files. 
// 
// 
//
// ===========================================================================
function StateMachine(smdir) {

	// State machine instance variables
	var obj = {};
	obj.states = {};
	obj.state = [];
	obj.conditions = {};

	// Read state definitions from CSV file
	obj.load_states = function(fname) {

		// Convert CSV to 2D array
		var f = new File(fname);
		f.open();
		cells = csv2array(f.readstring(65536), ',');
		f.close();

		// Iterate row-wise for state specifications
		for (var i = 0; i < cells.length; i++) {
			name = cells[i][0];
			outlist = cells[i].slice(1);
			outlist = outlist.filter(Boolean);		// Remove empty elements
			if (name !== '' && name != null) {		// Ignore empty rows
				if (obj.states.hasOwnProperty(name)) 
					obj.states[name].outlist_end = outlist;
				else
					obj.states[name] = State(name, outlist);
			}
		}
		// Print state outputs
		if (verbose > 5) {
			post(separator);
			for (var key in obj.states) {
				obj.states[key].post();
				post(separator);
			}
		}
	}

	// Read conditions and array sizes from CSV file
	obj.load_conditions = function(fname) {
		// Convert CSV to 2D array
		var f = new File(fname);
		f.open();
		cells = csv2array(f.readstring(65536), ',');
		f.close();
		// Get conditions from first column, array lengths from second
		for (var i = 0; i < cells.length; i++) {
			oscpath = cells[i][0];
			if (oscpath == '')
				break;
			len = cells[i][1];
			obj.conditions[oscpath] = new Array(len);
			for (var j = 0; j < len; j++)
				obj.conditions[oscpath][j] = 0;
		}
	}

	// Printing helpers
	obj.post_states = function() {
		post("STATES:\n");
		for (var key in obj.states)
			post("- " + key + "\n");
	}
	obj.post_conditions = function() {
		post("CONDITIONS:\n");
		for (key in obj.conditions) 
			post("- " + key + ": " + obj.conditions[key] + "\n");
	}

	obj.reset = function() {
		// Reset conditions
		for (key in obj.conditions) {
			for (var i = 0; i < obj.conditions[key].length; i++)
				obj.conditions[key][i] = 0;
		}
		// Clear condition history for each state
		for (key in obj.states) 
			obj.states[key].history = ConditionHistory();
	}

	// Begin state defined in first row of CSV 
	obj.init = function() {
		obj.state = obj.states[Object.keys(obj.states)[0]];
		if (verbose) post(separator);
		obj.state.begin(obj.conditions);
	}

	// Handle incoming OSC messages
	obj.handle_osc = function(path, args) {

		// Make sure the OSC path is relevant
		if (!obj.conditions.hasOwnProperty(path)) {
			error('Unhandled OSC path \'' + path + '\'\n');
			return;
		}
		// Make sure we have an index and a value
		if (args.length < 2) {
			error('Usage: ' + path + ' [index] [value]\n');
			return;
		}
		// Make sure the index is valid
		if (args[0] < 1 || args[0] > obj.conditions[path].length) {
			error('Invalid index ' + args[0] + ' for path \'' + path + '\'\n');
			return;
		}
		if (verbose) {
			post("OSC IN: " + path + ' ' + args[0] + ' ' + args[1] + '\n');
		}

		// Update the current conditions
		obj.conditions[path][args[0]-1] = args[1];
		if (verbose > 4) {
			obj.post_conditions();
		}

		// Evaluate transitions
		var sdestname = obj.state.update(obj.conditions);

		// Transition to the next state 
		if (sdestname) {
			obj.state = obj.states[sdestname];
			obj.state.begin(obj.conditions);
		}	 	
	}

	// Parse CSV files
	post('Creating state machine from directory \'states.csv\' and \'conditions.csv\' in ' + fullpath(smdir) + '\n');
	obj.load_states(fullpath(smdir) + '/states.csv');
	obj.load_conditions(fullpath(smdir) + '/conditions.csv');

	//
	if (verbose) {
		obj.post_states();
		obj.post_conditions();
	}
	return obj;
}

// ===========================================================================
//
// State
//
// Class representating states, defined by name and array of output messages.
//
// ===========================================================================
function State(name, outlist) {

	var obj = {};			// This state
	obj.name = name;		// This state's name
	obj.outlist = outlist;	// Array of this state's output messages (strings)
	obj.outlist_end = [];
	obj.history = ConditionHistory();

	obj.post = function() {
		post("STATE \'" + obj.name + "\'\n");
		post("- OUTPUTS (Initial): \n")
		for (var i = 0; i < obj.outlist.length; i++)
			post("-- " + obj.outlist[i] + '\n');	
		if (obj.outlist_end.length > 0) {
			post("- OUTPUTS (Final): \n")
			for (var i = 0; i < obj.outlist_end.length; i++) {
				post("-- " + obj.outlist_end[i] + '\n');	
			}
		}	
	}
	
	obj.send_actions = function(outlist) {
		// Send state actions to left outlet
		for (var i = 0; i < outlist.length; i++) {
			var tokens = outlist[i].split(' ');
			var arr = [];
			for (var j = 0; j < tokens.length; j++) {
				if (isNaN(+tokens[j]))
					arr.push(tokens[j]);
				else
					arr.push(+tokens[j]);
			}
			outlet(0, arr);
		}	
	}

	// Outputs action messages associated with this state
	obj.begin = function(conditions) {
		
		if (verbose) {
			post("BEGIN STATE \'" + obj.name + "\'\n");
			post(separator);
		}
		
		// Send state to right outlet
		outlet(1, obj.name);

		// Add state's initial conditions to the condition history
		obj.history.update(conditions);
		if (verbose > 4) {
			obj.history.post();
		}

		if (verbose > 4) 
			post(separator);

		// Send this state's actions
		obj.send_actions(obj.outlist);

		// Pass to callback function if it exists
		try {
			eval("begin_" + obj.name + "()");
		}
		catch(err) {
			// No callback
		}
	}

	// State condition handler. Checks if any transition conditions are met and returns
	// destination state name or false.
	obj.update = function(conditions) {

		var dest = false;

		obj.history.update(conditions);
		if (verbose > 5) {
			obj.history.post();
		}

		// Pass to callback function if it exists
		try {
			var args = obj.history.get_updates();
			dest = eval("update_" + obj.name + "(args[0], args[1], args[2], args[3])");
			post("update_" + obj.name + "() returned state \'" + cb_dest + '\'\n');
		}
		catch(err) {
			// No callback
			post("no callback update_" + obj.name + "()\n");
		}

		if (verbose)
			post(separator);

		if (dest) {

			// Send state end actions
			obj.send_actions(obj.outlist_end);

			// Pass to callback function if it exists
			try {
				eval("end_" + obj.name + "()");
			}
			catch(err) {
				// No callback
			}
		}

		return dest;
	}
	//
	return obj;
}

// ===========================================================================
//
// ConditionHistory
//
// Keeps track of changing conditions, flagging when they've changed and which
// changes are unique.
//
// ===========================================================================
function ConditionHistory() {

	var obj = {};
	obj.history = {};
	obj.changes = {};
	obj.unique = {};

	obj.get_updates = function() {
		var retval = ['', 0, 0, false];
		for (key in obj.changes) {
			var len = obj.changes[key].length;
			for (var i = 0; i < len; i++)
				if (obj.changes[key][i] == 1) {
					retval[0] = key;
					retval[1] = i;
					retval[2] = obj.history[key][obj.history[key].length-1][i];
					retval[3] = obj.unique[key];
				}
		}
		return retval;
	}

	obj.post = function() {
		post("HISTORY:\n");
		for (var key in obj.history) {
			for (var i = 0; i < obj.history[key].length; i++)
				post("-- " + key + "[" + i + "]: " + obj.history[key][i] + '\n');
		}
		post("CHANGES:\n");
		for (var key in obj.changes) {
			post("-- " + key + ": ");
			post(obj.changes[key] + '\n');
		}
		post("UNIQUE:\n");
		for (var key in obj.unique) {
			post("-- " + key + ": " + obj.unique[key] + '\n');
		}
	}

	// Check the condition history to see if these values have occurred before
	// (ignores the most recent entry)
	obj.in_history = function(key, values) {
		if (!obj.history.hasOwnProperty(key))
			return false;
		var found = false;
		var row = 0;
		while (!found && row < obj.history[key].length-1) {
			var rowvals = obj.history[key][row];
			var rowmatch = true;
			for (var i = 0; i < rowvals.length; i++) {
				if (rowvals[i] != values[i])
					rowmatch = false;
			}
			found = rowmatch;
			row += 1;
		}
		return found;
	}

	obj.update = function(conditions) {
		
		for (var key in conditions) {

			len = conditions[key].length;
			
			// If there's no history of this key, create one
			if (!obj.history.hasOwnProperty(key)) {
				obj.history[key] = [];
				obj.changes[key] = new Array(len);
				for (var i = 0; i < len; i++) {
					obj.changes[key][i] = 1;
				}
				obj.unique[key] = true;
			}
			// Otherwise, check to see which values have changed
			else {
				var histlen = obj.history[key].length;
				for (var i = 0; i < len; i++) {
					if (obj.history[key][histlen-1][i] != conditions[key][i])
						obj.changes[key][i] = 1;
					else 
						obj.changes[key][i] = 0;
				}
			}
			// If flagged for changes, add a new row to the condition history
			var updated = false;
			for (var i = 0; i < len; i++) {
				if (obj.changes[key][i] != 0) {
					obj.history[key].push([]);
					for (var i = 0; i < conditions[key].length; i++) {
						var histlen = obj.history[key].length;
						obj.history[key][histlen-1].push(conditions[key][i]);
					}
					updated = true;
				}
			}
			// Check if the current conditions are unique
			if (updated)
				obj.unique[key] = !obj.in_history(key, conditions[key]);
			else
				obj.unique[key] = false;
		}
	}

	return obj;
}

// CSV parser lifted from Stack Overflow:
// https://stackoverflow.com/questions/1293147/javascript-code-to-parse-csv-data
function csv2array(strData, strDelimiter) {
    // Check to see if the delimiter is defined. If not,
    // then default to comma.
    strDelimiter = (strDelimiter || ",");
    // Create a regular expression to parse the CSV values.
    var objPattern = new RegExp((
            // Delimiters.
            "(\\" + strDelimiter + "|\\r?\\n|\\r|^)" +
            // Quoted fields.
            "(?:\"([^\"]*(?:\"\"[^\"]*)*)\"|" +
            // Standard fields.
            "([^\"\\" + strDelimiter + "\\r\\n]*))"
        ), "gi");
    // Create an array to hold our data. Give the array
    // a default empty first row.
    var arrData = [[]];
    // Create an array to hold our individual pattern
    // matching groups.
    var arrMatches = null;
    // Keep looping over the regular expression matches
    // until we can no longer find a match.
    while (arrMatches = objPattern.exec(strData)) {
        // Get the delimiter that was found.
        var strMatchedDelimiter = arrMatches[1];
        // Check to see if the given delimiter has a length
        // (is not the start of string) and if it matches
        // field delimiter. If id does not, then we know
        // that this delimiter is a row delimiter.
        if (strMatchedDelimiter.length && strMatchedDelimiter !== strDelimiter) {
            // Since we have reached a new row of data,
            // add an empty row to our data array.
            arrData.push([]);
        }
        var strMatchedValue;
        // Now that we have our delimiter out of the way,
        // let's check to see which kind of value we
        // captured (quoted or unquoted).
        if (arrMatches[2]) {
            // We found a quoted value. When we capture
            // this value, unescape any double quotes.
            strMatchedValue = arrMatches[2].replace(new RegExp("\"\"", "g"), "\"");
        } 
        else {
            // We found a non-quoted value.
            strMatchedValue = arrMatches[3];
        }
        // Now that we have our value string, let's add
        // it to the data array.
        arrData[arrData.length - 1].push(strMatchedValue);
    }
    // Return the parsed data.
    return(arrData);
}

function match_array(arr1, arr2) {
	if (arr1.length !== arr2.length)
		return false;
	for (var i = 0; i < arr1.length; i++) {
		if (arr1[i] != arr2[i])
			return false;
	}
	return true;
}