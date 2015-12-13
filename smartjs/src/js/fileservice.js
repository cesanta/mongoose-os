var FileService = function(clubby) {
  clubby.oncmd("/v1/File.ReadAll", function(cmd) {
    if (cmd.args === undefined) {
      throw "Need argument";
    }
    if (cmd.args.name === undefined) {
      throw "Need name";
    }
    var f = File.open(cmd.args.name);
    if (f === null) {
      throw "Failed to open";
    }
    var data = f.readAll();
    f.close();
    return data;
  });
};
