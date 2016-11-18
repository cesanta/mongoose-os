var FileService = function(mgRPC) {
  mgRPC.oncmd("/v1/File.ReadAll", function(cmd) {
    if (cmd.args === undefined) {
      throw "Need argument";
    }
    if (cmd.args.name === undefined) {
      throw "Need name";
    }
    return File.read(cmd.args.name);
  });
};
