/* 
 	 videoctrl.js
 	
 	 Front-end interface for controlling jit.qt.movie instances using a 
 	 standardized OSC message protocol.
 	
 	 Usage: 
 		[js videoctrl.js /[assetfolder]]
 	
 	 Example:
 		[js videoctrl.js /video]
 	
 	 Responds to OSC messages conforming to:
 	 	/[assetfolder] [clip #] [one-shot/loop]	
 	
 	 Asset folders should contain MP4 files, which are controlled through
 	 this object by clip numbers assigned alphabetically. The asset folder
 	 may contain subdirectories of MP4 files.
 	
 	 Examples:
 	
 	 Play third clip in /assets/video (once):
 	 	/assets/video 3		
 		
 	 Play second clip in /assets/video (loop):
 		/assets/video 2 1
 	 
 	 Advance current clip (stop and send EOF):
 	 	/assets/video 0
 		/assets/video advance
 	
 	 Stop current clip (don't send EOF):
 		/assets/video -1
 		/assets/video stop
 	
 	 Pause current clip:
 	 	/assets/video pause
 	
 	 Resume current clip:
 		/assets/video resume
 	
 	 Set gain of video's audio track (in dB):
 		/assets/video gain -20

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

include("assetctrl");

inlets = 1;
outlets = 2;

// Make a 2-D asset path dictionary keyed by relative directory and cue number
var asset_dir = jsarguments.slice(1);
var file_dict = get_asset_dict(asset_dir);
// post_asset_dict(file_dict);

// Create asset controller instance
ac = VideoController(		// Callbacks:
		ctlout_loop,		// - Loop
		ctlout_start,		// - Start
		ctlout_pause, 		// - Pause
		ctlout_resume, 		// - Resume
		ctlout_stop,		// - Stop
		ctlout_eof);		// - EOF

setoutletassist(0, "to jit.movie instance");
setoutletassist(1, "EOF");

// RegExp to test for valid messages (those starting with a slash, followed
// by the asset directory name) and globals (addressed to every outlet)
var re_valid = new RegExp('^' + asset_dir);	
var re_global = new RegExp('^' + asset_dir + '$'); 

// Main message handler
function anything() {

	// Handle messages that start with the asset directory
	if (!re_valid.test(messagename))
		return;

	// Ignore OSC if path doesn't match asset directory or subdirectory
	if (!(file_dict.hasOwnProperty(messagename) || re_global.test(messagename)))
		return;
	
	// If the asset controller doesn't handle the message
	if (!ac.handle_osc(messagename, arguments)) { 

		switch (arguments[0]) {
			case 'advance':
				advance();
				break;
			case 'stop':
				stop();
				break;
			case 'pause':
				pause();
				break;
			case 'resume':
				resume();
				break;
			case 'gain':
				if (arguments.length > 1)
					ctlout_gain(arguments[1]);
			default:
				break;
		}
	}
}

// EOF Handling:
// -------------
function bang() {
	ac.eof_in();
}

// Global commands:
// ----------------
// Handle stop 
function stop() {
	ac.stop();
}

// Handle advance (stop and send EOF)
function advance() {
	ac.advance();
}

// Handle pause
function pause() {
	ctlout_pause();
}

// Handle resume
function resume() {
	ctlout_resume();
}

// Video control output handlers:
// ------------------------------
function ctlout_gain(value) {
	outlet(0, 'vol', Math.pow(10, value/20));
}

// Set loop mode 
function ctlout_loop(do_loop) {
	outlet(0, 'loop', do_loop);
}

// Start clip
function ctlout_start(key, clipnum) {
	outlet(0, 'read', file_dict[key][clipnum]);
}

// Pause clip
function ctlout_pause() {
	outlet(0, 'stop');
}

// Resume clip
function ctlout_resume() {
	outlet(0, 'start');
}

// Stop clip
function ctlout_stop() {
	outlet(0, 'dispose');
}

// Sends an OSC message to the state machine, flagging this clip as played
function ctlout_eof(key, clipnum) {
	outlet(1, key, clipnum, 1);
}

