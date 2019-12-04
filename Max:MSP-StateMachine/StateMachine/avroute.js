inlets = 1;
outlets = 3;

function anything() {
	var toks = messagename.split('/');
	if (toks[1] === 'audio') 
		outlet(0, arrayfromargs(messagename, arguments));
	else if (toks[1] === 'video')
		outlet(1, arrayfromargs(messagename, arguments));
	else 
		outlet(2, arrayfromargs(messagename, arguments));
}