/*
  	pathutil.js

  	Common utilities used by statemachine.js and assetctrl.js.

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

// Print separator
var separator = "================================\n";

// Get a 2D dictionary keyed by relative directory and number 1,...,N for each asset
// file in the directory (alphabetical). Recommending naming files 1_filename.ext, 
// ..., N_filename.ext so names are both descriptive and ordered.
function get_file_dict(paths) {
	
	var dict = {};

	for (var i = 0; i < paths.length; i++) {

		// Create a dict entry for this directory
		oscpath = paths[i];
		dict[paths[i]] = {};

		// Get the full path to the directory
		var assetpath = fullpath(paths[i]);

		// Assign a cue number to each file in the directory
		var cue_num = 1;
		var adir = new Folder(assetpath);
		while (!adir.end) {
			if (adir.filetype != '') {
				dict[paths[i]][cue_num] = assetpath + '/' + adir.filename;
				cue_num++;
			}
			adir.next();
		}
		adir.close();
	}

	// Post cued files
	for (var dname in dict) {
		if (dict.hasOwnProperty(dname)) {
			for (var fname in dict[dname]) {
				if (dict[dname].hasOwnProperty(fname)) {
					post(dname + ' ' + fname + '\n');
				}
			}
		}
	}
	return dict;
}

// Make a full path to the specified relative asset path and clip number.
function fullpath(assetpath, clipnum, fileext) {

	// Remove a leading slash if present
	if (assetpath[0] == '/')
		assetpath = assetpath.slice(1);
	
	// Max gives access to the full path to the patcher file that loaded this script, 
	// which is something like 'Macintosh HD: /Users/.../projectdir/main.maxpat', so
	// we get the present working directory by triming disk name and patcher filename
	var p = this.patcher.filepath;	
	p = p.substring(p.indexOf(':') + 1, p.lastIndexOf('/'));

	// Make the full path to the clip or directory
	if (typeof clipnum === 'undefined')
		return p + '/' + assetpath;
	else if (typeof fileext != 'undefined')
		return p + '/' + assetpath + '/' + clipnum +  '.' + fileext;
}