include("statemachine");

inlets = 1;
outlets = 2;

verbose = 7;

var dir_sm = '/sm';
if (jsarguments.length > 1)
	dir_sm = jsarguments[1];

var sm = StateMachine(dir_sm);
var initialized = false;

function init() {
	sm.init();
	initialized = true;
}

function reset() {
	sm.reset();
}

function conditions() {
	sm.post_conditions();
}

function anything() {
	// Make sure the state machine has been initialized
	if (initialized === false) {
		error("Ignoring \'" + messagename + "\'. Initialize first\n");
		return;
	}
	// Delegate OSC handling to the state machine
	sm.handle_osc(messagename, arguments);
}

// ======================
// State Update Callbacks
// ======================

function update_idle(key, idx, val, unique) {
	if (key != '/retrieval') {
		return;
	}
	if (val == 0) {
		return;
	}
	if (idx == 0) {
		return 'retrieval1';
	}
	else if (idx == 1) {
		return 'retrieval2';
	}
}

function update_retrieval1(key, idx, val, unique) {
	if (key != '/video/object') {
		return;
	}
	if (val == 0) {
		return;
	}
	if (sm.conditions[key][0] == 1 & sm.conditions[key][1] == 1) {
		return 'exit';
	}
	else {
		return 'idle';
	}	
}

function update_retrieval2(key, idx, val, unique) {
	if (key != '/video/object') {
		return;
	}
	if (val == 0) {
		return;
	}
	if (sm.conditions[key][0] == 1 & sm.conditions[key][1] == 1) {
		return 'exit';
	}
	else {
		return 'idle';
	}	
}

function update_exit(key, idx, val, unique) {
	if (key != '/video') {
		return;
	}
	return 'fin';
}
