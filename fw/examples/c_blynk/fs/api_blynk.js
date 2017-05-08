let Blynk = {
	_send: ffi('void blynk_send(void *, int, int, char *, int)'),

	// ## `Blynk.send(conn, type, msg)`
	send: function(conn, type, msg) {
		this._send(conn, type, 0, msg, msg.length);
	},

	// ## `Blynk.setHandler(handler)`
	// Example:
	// ```javascript
	// Blynk.setHandler(function(conn, cmd, pin, val) {
	//	 print(cmd, pin, val);
	// }, null);
	// ```
	setHandler: ffi('void blynk_set_handler(void (*)(void *, char *, int, int, userdata), userdata)'),
};
