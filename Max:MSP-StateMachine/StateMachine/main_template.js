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
