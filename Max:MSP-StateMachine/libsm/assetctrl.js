/*
  	assetctrl.js
 
	Contains functions for creating dictionaries of assets by parsing asset 
	directories, and a class for mapping OSC messages to video/audio control 
	callbacks.
 
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

// Merge two dictionaries (doesn't check for duplicates)
function merge_dict(dict1, dict2) {
	dict = {};
	for (var key in dict1) {
		dict[key] = dict1[key];
	}
	for (var key in dict2) {
		dict[key] = dict2[key];
	}
	return dict;
}

// Post a 2-D asset path dictionary keyed by relative directory and cue number
function post_asset_dict(dict) {
	for (var dir in dict) {
		for (var cue in dict[dir]) {
			post("assets[" + dir + "][" + cue + "]: " + dict[dir][cue] + "\n");
		}
	}
}

// Make a 2-D asset path dictionary keyed by relative directory and cue number.
// Parse sub-directories recursively and merge dictionaries.
function get_asset_dict(path) {

	var dict = {};
	var fpath = fullpath(path);
	var adir = new Folder(fpath);
	var cue_num = 1;

	while (!adir.end) {

		// Handle folders (recursively)
		if (adir.filetype == 'fold') {
			dict = merge_dict(dict, get_asset_dict(path + '/' + adir.filename));
		}
		// Handle files
		else if (adir.filetype != '') {
			// Create a dict entry if we don't have one yet
			if (Object.keys(dict).length === 0) 
				dict[path] = {};
			// Add a cue number key for this file and store the full path
			dict[path][cue_num] = fpath + '/' + adir.filename;
			cue_num++;
		}
		adir.next();
	}
	adir.close();
	return dict;
}

// Handles incoming OSC messages and delegates to the provided callback 
// functions for controlling playback of jit.movie and jit.qt.movie objects 
// and generating formatted EOF messages.
function VideoController(cb_loop, cb_start, cb_pause, cb_resume, cb_stop, cb_eof) {

	var obj = {};

	// Current asset values
	obj.path;	// OSC path (same name as asset folder path)
	obj.clip;	// Clip number
	obj.loop;	// Whether the clip is looping

	// Handlers control outputs
	obj.cb_loop = cb_loop;
	obj.cb_start = cb_start;
	obj.cb_pause = cb_pause;
	obj.cb_resume = cb_resume;
	obj.cb_stop = cb_stop;
	obj.cb_eof = cb_eof;

	// Reset current clip info
	obj._clear = function() {
		obj.path = '';
		obj.clip = 0;
		obj.loop = 0;
	}

	// Start a clip and store info
	obj.start = function(path, clip, loop) {
		if (!(obj.path == '' && obj.clip == 0)) 
			obj.advance();
		obj.cb_loop(loop);
		obj.cb_start(path, clip);
		obj.path = path;
		obj.clip = clip;
		obj.loop = loop;
	}

	// Stop a clip and clear info
	obj.stop = function() {
		obj.cb_stop();
		obj._clear();
	}

	// Stop a clip and send EOF
	obj.advance = function() {
		var _path = obj.path;
		var _clip = obj.clip;
		obj.stop();
		obj.cb_eof(_path, _clip);
	}

	// Send EOF
	obj.eof_in = function() {
		obj.cb_eof(obj.path, obj.clip);
		if (!obj.loop) 
			obj._clear();
	}

	// OSC handler
	obj.handle_osc = function(path, args) {

		// Make sure we have arguments
		if (args.length < 1)
			return false;

		// Stop the current clip
		if (args[0] == -1 || args[0] == 'stop')
			obj.stop();

		// Advance the currnet clip 
		else if (args[0] == 0 || args[0] == 'advance')
			obj.advance()

		// Pause the current clip
		else if (args[0] == 'pause')
			obj.cb_pause();

		// Resume the current clip
		else if (args[0] == 'resume')
			obj.cb_resume();
		
		// Start a clip, one-shot/looped
		else if (args[0] > 0) {
			if (args.length > 1 && args[1] == 1)
				obj.start(path, args[0], 1);
			else
				obj.start(path, args[0], 0);
		}
		else 
			return false;
		return true;
	}

	obj._clear();
	return obj;
}

// Handles incoming OSC messages and delegates to the provided callback 
// functions for controlling playback of sfplay~ objects and generating
// and generating formatted EOF messages.
function AudioController(cb_gain, cb_loop, cb_start, cb_pause, cb_resume, cb_stop, cb_eof) {

	var obj = {};

	// Current asset values
	obj.path;	// OSC path (same name as asset folder path)
	obj.clip;	// Clip number
	obj.loop;	// Whether the clip is looping

	// Queued asset values
	obj.path_next;
	obj.clip_next;
	obj.loop_next;

	// Whether we're ignoring an EOF input (true) or awaiting EOF to send
	// our own formatted EOF message (false)
	obj.suppress_eof = false;

	// Handlers control outputs
	obj.cb_gain = cb_gain;
	obj.cb_loop = cb_loop;
	obj.cb_start = cb_start;
	obj.cb_pause = cb_pause;
	obj.cb_resume = cb_resume;
	obj.cb_stop = cb_stop;
	obj.cb_eof = cb_eof;

	// Reset current clip info
	obj._clear = function() {
		obj.path = '';
		obj.clip = 0;
		obj.loop = 0;
		obj.suppress_eof = false;
	}

	// Set a clip to play on the current clip's EOF
	obj._enqueue = function(path, clip, loop) {
		obj.path_next = path;
		obj.clip_next = clip;
		obj.loop_next = loop;
	}

	// Clear the queued clip
	obj._dequeue = function() {
		obj.path_next = '';
		obj.clip_next = 0;
		obj.loop_next = 0;
	}

	// Start a clip and store info
	obj.start = function(path, clip, loop) {
		// If there's currently no clip, start immediately
		if (obj.path == '' && obj.clip == 0) {
			obj.path = path;
			obj.clip = clip;
			obj.loop = loop;
			obj.cb_loop(obj.path, obj.loop);
			obj.cb_start(obj.path, obj.clip);
		}
		// Otherwise enqueue it to start on current clip's EOF, and advance
		else {
			obj._enqueue(path, clip, loop);
			obj.advance();
		}
	}

	// Stop a clip and clear info
	obj.stop = function() {
		obj.suppress_eof = true;
		obj.cb_stop(obj.path);
	}

	// Stop a clip and send EOF
	obj.advance = function() {
		obj.suppress_eof = false;
		obj.cb_stop(obj.path);
	}

	// Send formatted EOF message in response to a received EOF signal
	obj.eof_in = function() {
		// Send EOF and clear current clip info
		if (!obj.suppress_eof) 
			obj.cb_eof(obj.path, obj.clip);
		obj._clear();
		// If there's a next clip awaiting the previous clip's EOF, start it
		if (obj.path_next != '' && obj.clip_next != 0) {
			obj.start(obj.path_next, obj.clip_next, obj.loop_next);
			obj._dequeue();
		}
	}

	// OSC handler
	obj.handle_osc = function(path, args) {

		// Make sure we have arguments
		if (args.length < 1)
			return false;

		// Set gain
		if (args[0] == 'gain' && args.length > 1) 
			obj.cb_gain(path, args[1]);

		// Stop the current clip
		else if (args[0] == -1 || args[0] == 'stop')
			obj.stop();

		// Advance the currnet clip 
		else if (args[0] == 0 || args[0] == 'advance')
			obj.advance()

		else if (args[0] == 'pause') 
			obj.cb_pause(path);

		else if (args[0] == 'resume')
			obj.cb_resume(path);
		
		// Start a clip, one-shot/looped
		else if (args[0] > 0) {
			if (args.length > 1 && args[1] == 1)
				obj.start(path, args[0], 1);
			else
				obj.start(path, args[0], 0);
		}
		else 
			return false;
		return true;
	}

	obj._clear();
	obj._dequeue();
	return obj;
}

// Deprecated:
// -----------
// Asset controller instance. Handles incoming OSC messages and delegates
// to the provided callback functions for controlling playback of audio
// and video player objects and generating EOF messages.
function AssetController(cb_loop, cb_start, cb_stop, cb_advance, cb_eof) {

	var obj = {};

	// Current asset values
	obj.path;		// OSC path (same name as asset folder path)
	obj.clip = 0;	// Clip number

	// Handlers control outputs
	obj.cb_loop = cb_loop;
	obj.cb_start = cb_start;
	obj.cb_stop = cb_stop;
	obj.cb_advance = cb_advance;
	obj.cb_eof = cb_eof;

	// Handle kill message:
	// Stop the current asset, but first set the clip number to zero so the EOF
	// kicked back from the (a/v) player is ignored and we don't update the state
	// machine. If the state machine is sending 'kill' after a reset, at this point, 
	// it will already have cleared all flags, so we don't want to touch them. 
	obj.kill = function() {d
		obj.clip = 0;
		obj.cb_stop(obj.path);
	}

	// EOF handler
	obj.eof_in = function() {
		if (obj.clip != 0) {
			var oldclip = obj.clip;
			obj.clip = 0;
			obj.cb_eof(obj.path, oldclip);
		}
	}

	// Advance handler
	obj.advance = function() {
		if (obj.clip != 0)
			obj.cb_advance(obj.path, obj.clip);
	}

	// OSC handler
	obj.handle_osc = function(path, args) {

		var newclip;
		var start;
		var loop;

		// Parse arguments:
		// ----------------
		// Single argument
		if (args.length == 1) {
			// Stop current clip
			if (args[0] == 0) {
				newclip = obj.clip;
				start = 0;
			}
			// Start specified clip (default: one-shot)
			else {
				newclip = args[0];
				start = 1;
			}
			loop = 0;
		}
		// Multple agruments
		else if (args.length > 1) {
			newclip = args[0];			// Specified clip
			start = args[1];			// Start/stop spec.
			loop = 0;
			if (args.length > 2) 		// Loop spec.
				loop = args[2];
		}

		// Start/Stop clips:
		// -----------------
		// If we're starting a video
		if (start && newclip) {

			// If there's a clip playing advance it
			if (obj.clip != 0)
				obj.advance(obj.path, obj.clip);

			// Set loop mode and start the new clip
			obj.cb_loop(path, loop);
			obj.cb_start(path, newclip);

			// Keep track of the clip we're playing until EOF
			obj.path = path;
			obj.clip = newclip;		
			return true;
		}
		else if (newclip == obj.clip)
			obj.cb_stop(obj.path, obj.clip);

		return false;
	}

	return obj;
}
