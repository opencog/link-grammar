Module.preRun = Module.preRun || []
Module.preRun.push(function () {
  // mount `data` dir
  FS.mkdir('/data')
  FS.mount(NODEFS, {root: __dirname + '/data'}, '/data');

  function readline () {
    var BUFSIZE = 256;
    var buf = Buffer.alloc ? Buffer.alloc(BUFSIZE) : new Buffer(BUFSIZE);
    var bytesRead = 0;

    var fd = process.stdin.fd;
    var usingDevice = false;
    if (process.platform !== 'win32') {
      // Linux and Mac cannot use process.stdin.fd (which isn't set up as sync)
      try {
        fd = fs.openSync('/dev/stdin', 'r');
        usingDevice = true;
      } catch (e) {}
    }

    bytesRead = fs.readSync(fd, buf, 0, BUFSIZE, null);

    if (usingDevice) {
      fs.closeSync(fd);
    }

    return buf.slice(0, bytesRead).toString('utf-8');
  }

  // read from stdin
  TTY.default_tty_ops.get_char = function (tty) {
    if (!tty.input.length) {
      tty.input = intArrayFromString(readline(), true);
      tty.input.push(null); // after emitting any [true] input, emit `null` to signal empty buffer
    }
    return tty.input.shift();
  };

  // override to write chars to stdout/stderr *immediately*, without waiting for flush
  // stdout
  TTY.default_tty_ops.put_char = function (tty, val) {
    process.stdout.write(UTF8ArrayToString([val], 0));
  };
  TTY.default_tty_ops.flush = function (tty) {};
  // stderr
  TTY.default_tty1_ops.put_char = function (tty, val) {
    process.stderr.write(UTF8ArrayToString([val], 0));
  };
  TTY.default_tty1_ops.flush = function (tty) {};
})
