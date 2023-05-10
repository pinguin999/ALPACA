Module["preRun"].push(function () {
    addRunDependency('syncfs')

    FS.mkdir('/working1')
    FS.mount(IDBFS, {}, '/working1')
    FS.syncfs(true, function (err) {
      if (err) throw err
      removeRunDependency('syncfs')
      console.log("FS Synced")
    })
  });
