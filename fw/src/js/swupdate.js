var SWUpdate = function(mgRPC) {
  mgRPC.oncmd("/v1/SWUpdate.ListSections", function() {
    var files = File.list('.');
    var r = [];
    for (var i in files) {
      r.push("file/" + files[i]);
    }
    return r;
  });
  // TODO(lsm): Move to the File service
  mgRPC.oncmd("/v1/SWUpdate.Delete", function(cmd, done) {
    if (typeof(cmd.args.section) !== 'string') {
      done("Need 'section' argument", 1);
    } else if (cmd.args.section.indexOf("file/") !== 0) {
      done("Only files on FS are supported", 1);
    } else {
      var name = cmd.args.section.substring(5);
      done('result', File.remove(name));
    }
  });
  mgRPC.oncmd("/v1/SWUpdate.Update", function(cmd, done) {
    if (cmd.args === undefined) {
      done("Need argument", 1);
      return;
    }
    if (cmd.args.section === undefined ||
        (cmd.args.blob === undefined && cmd.args.blob_url === undefined)) {
      done("Need section and blob", 1);
      return;
    }
    if (cmd.args.section.indexOf("file/") !== 0) {
      done("Only files on FS are supported", 1);
      return;
    }
    var write = function(data) {
      var file = File.open(cmd.args.section.substring(5), 'w');
      if (file === null) {
        done("Failed to open the file", 1);
        return;
      }
      file.write(data);
      file.close();
      setTimeout(function() {
        Sys.reboot();
      }, 3000);
      done();
    };
    if (cmd.args.blob !== undefined) {
      write(cmd.args.blob);
    } else if (cmd.args.blob_url !== undefined) {
      Http.get(cmd.args.blob_url, function(data, err) {
        if (err) {
          done(err, 1);
        } else {
          write(data);
        }
      });
    } else {
      done("Invalid combination of arguments", 1);
    }
  });
};
