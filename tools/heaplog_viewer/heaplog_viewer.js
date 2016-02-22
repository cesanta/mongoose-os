"use strict";

(function(){

  var logger;

  var guiDeltaItems = [];

  var cellData;

  /* Enables additional checks that affect performance */
  var DEBUG = false;

  var BYTES_PER_ROW = 1024;
  var CELLS_PER_ROW = 128;
  var BYTES_PER_CELL = (BYTES_PER_ROW / CELLS_PER_ROW);

  var OVERHEAD_BYTES_PER_ALLOC = 8;

  var ItemType = {
    NONE: 0,
    MALLOC: 1,
    REALLOC: 2,
    FREE: 3
  }

  var AllocAction = {
    ALLOC: 0,
    FREE: 1,
  }

  var tbl = jQuery("#map_tbl")[0];
  var i, x, y;

  tbl.style.border = '1px solid black';
  tbl.cellPadding = 0;
  tbl.cellSpacing = 0;
  tbl.style.tableLayout = 'fixed';

  document.getElementById('file-input').addEventListener(
    'change', readLogFile, false
  );

  document.getElementById('log_sel').addEventListener(
    'change', onLogItemChanged, false
  );

  function setHint(msg) {
    jQuery("#hint_input").val(msg);
  }

  function setHintAddr(addr) {
    jQuery("#hint_addr_input").val(
      addr === undefined ? "" : addr.toString(16)
    );
  }

  function allocOnMouseOver(x, y, on) {
    if (cellData[y][x].alloc) {
      var alloc = cellData[y][x].alloc;
      drawAllocation(alloc, on);
      if (on){
        setHint(alloc.item.toString());
      } else {
        setHint("");
      }
    } else {
      setHint("");
    }

    if (on){
      setHintAddr(cellData[y][x].addr);
    } else {
      setHintAddr(undefined);
    }
  }

  function allocOnClick(x, y) {
    if (cellData[y][x].alloc) {
      var alloc = cellData[y][x].alloc;
      setLogItemIdx(alloc.item.idx);
    }
  }

  function guiRebuild() {
    var i;

    var heapStart = logger.getHeapStart();
    var heapEnd = logger.getHeapEnd();
    var heapSize = heapEnd - heapStart;
    var statTotal = logger.getStatTotal();

    var curAddr;

    function setNextItemFromList(items) {
      if (items.length > 0) {
        var idx = items[0].idx;
        var i;
        var curItem = logger.getCurItem();

        /*
         * check if we're currently at some of the items from the list.
         * if so, then set the next one.
         */
        for (i = 0; i < items.length - 1; i++) {
          if (items[i].idx == curItem.idx) {
            idx = items[i + 1].idx;
            break;
          }
        }

        setLogItemIdx(idx);
      }
    }

    // populate heap properties
    jQuery("#heap_start").attr("value", logger.getHeapStart().toString(16));
    jQuery("#heap_end").attr("value", logger.getHeapEnd().toString(16));
    jQuery("#heap_size").attr("value", logger.getHeapSize());
    jQuery("#bytes_per_row").attr("value", BYTES_PER_ROW);
    jQuery("#bytes_per_cell").attr("value", BYTES_PER_CELL);

    jQuery("#max_heap_usage").html(statTotal.maxHeapUsage.value);
    jQuery("#max_heap_usage").click(function() {
      setNextItemFromList(statTotal.maxHeapUsage.items);
    })

    jQuery("#max_heap_usage_with_overhead").html(
      statTotal.maxHeapUsageWOvh.value
    );
    jQuery("#max_heap_usage_with_overhead").click(function() {
      setNextItemFromList(statTotal.maxHeapUsageWOvh.items);
    })

    jQuery("#max_alloc_cnt").html(statTotal.maxAllocCnt.value);
    jQuery("#max_alloc_cnt").click(function() {
      setNextItemFromList(statTotal.maxAllocCnt.items);
    })

    jQuery("#min_contiguous_block_size").html(statTotal.minChunkSize.value);
    jQuery("#min_contiguous_block_size").click(function() {
      setNextItemFromList(statTotal.minChunkSize.items);
    })

    // populate log window
    $("#log_sel option").remove();
    var select = jQuery("#log_sel")[0];
    for (i = 0; i < logger.getItemsCnt(); i++){
      var item = logger.getItem(i);
      var opt = document.createElement('option');
      opt.value = i;
      opt.innerHTML = item.toString();
      if (item.type == ItemType.NONE){
        opt.className = "log_item_type_none";
      }
      select.appendChild(opt);
    }

    // remove map
    while (tbl.rows.length > 0) {
      tbl.deleteRow(0);
    }

    cellData = [];

    // create new map of the appropriate size: one row represents 1 KB,
    // each cell represents 8 bytes.
    curAddr = heapStart;
    for (y = 0; y < (heapSize / BYTES_PER_ROW); y++){
      cellData[y] = [];
      var tr = tbl.insertRow();
      for(x = 0; x < CELLS_PER_ROW; x++){
        cellData[y][x] = {
          addr: curAddr,
        };
        var td = tr.insertCell();
        td.appendChild(document.createTextNode(' '));
        td.style.display = 'inline-block';
        td.style.width = '8px';
        td.style.height = '8px';
        td.style.fontSize = '1px';
        td.style.lineHeight = '1px';
        td.classList.add("log_area");

        // shadow out-of-heap area
        if (curAddr < heapStart || curAddr >= heapEnd) {
          td.classList.add("log_area_unavailable");
        }

        // set map mouse events (mouseover, mouseout, click)
        (function(x, y){
          td.addEventListener(
            'mouseover', function(e){
              allocOnMouseOver(x, y, 1);
            },
            false
          );

          td.addEventListener(
            'mouseout', function(e){
              allocOnMouseOver(x, y, 0);
            },
            false
          );

          td.addEventListener(
            'click', function(e){
              allocOnClick(x, y);
            },
            false
          );
        })(x, y);

        curAddr += BYTES_PER_CELL;
      }
    }

    guiLoggerStateApply();
  }

  function guiLoggerStateApply() {
    var i;
    var item = logger.getCurItem();

    if (guiDeltaItems.length < 100) {
      // the delta is small enough; apply it

      for (i = 0; i < guiDeltaItems.length; i++) {
        var allocItem = guiDeltaItems[i];

        if (allocItem.action == AllocAction.ALLOC) {
          drawAllocation(allocItem.alloc);
        } else if (allocItem.action == AllocAction.FREE) {
          hideAllocation(allocItem.alloc);
        } else {
          throw Error("wrong alloc allocItem action: " + allocItem.action);
        }
      }
    } else {
      // the delta is too long; it's better to just build the whole picture
      // from scratch

      var allocMap = logger.getAllocMap();
      var heapSize = logger.getHeapEnd() - logger.getHeapStart();
      var i;

      // clear all map
      for (y = 0; y < (heapSize / BYTES_PER_ROW); y++){
        var tr = tbl.insertRow();
        for(x = 0; x < CELLS_PER_ROW; x++){
          resetCell(tbl.rows[y].cells[x]);
          cellData[y][x].alloc = undefined;
        }
      }

      // populate new map
      var keys = allocMap.getAllocAddresses();
      for (i = 0; i < keys.length; i++) {
        var alloc = allocMap.getAllocByAddr(keys[i]);
        drawAllocation(alloc);
      }
    }

    guiDeltaItems.length = 0;

    // apply current stats
    jQuery("#cur_heap_usage").attr("value", item.stat.heapUsage);
    jQuery("#cur_heap_usage_with_overhead").attr(
      "value", item.stat.heapUsageWOvh
    );
    jQuery("#cur_alloc_cnt").attr("value", item.stat.allocCnt);
    jQuery("#cur_max_free_block_size").attr("value", item.stat.maxChunkSize);

    // select log item
    $("#log_sel").val( logger.getItemIdx() );
    $("#log_sel").focus();
  }

  function resetCell(cell) {
    cell.classList.remove("log_area_used");
    cell.classList.remove("log_area_shim");
    cell.classList.remove("log_area_used_begin");
    cell.classList.remove("log_area_used_end");
    cell.classList.remove("log_area_used_adj_top");
    cell.classList.remove("log_area_used_adj_bottom");
    cell.classList.remove("log_area_highlighted");
  }

  function getCellXYByAddr(addr) {
    addr -= logger.getHeapStart();
    var x = Math.floor((addr % BYTES_PER_ROW) / 8);
    var y = Math.floor(addr / BYTES_PER_ROW);

    return {
      x: x,
      y: y
    }
  }

  function drawAllocation(alloc, highlight) {
    var cellxy = getCellXYByAddr(alloc.addr);
    var addr;
    var cell;

    for (addr = alloc.addr; addr < alloc.addr + alloc.size; addr += BYTES_PER_CELL) {
      // get cell at current coords
      cell = tbl.rows[cellxy.y].cells[cellxy.x];

      // apply appropriate styles
      cell.classList.add("log_area_used");
      if (addr == alloc.addr) {
        cell.classList.add("log_area_used_begin");
      }

      if (alloc.shim) {
        cell.classList.add("log_area_shim");
      }

      if (highlight) {
        cell.classList.add("log_area_highlighted");
      } else {
        cell.classList.remove("log_area_highlighted");
      }

      //-- if the cell above belongs to the same allocation, clear the border
      if ((addr - BYTES_PER_ROW) >= alloc.addr) {
        cell.classList.add('log_area_used_adj_top');
        var cell2 = tbl.rows[cellxy.y - 1].cells[cellxy.x];
        cell2.classList.add('log_area_used_adj_bottom');
      }

      // set celldata
      cellData[cellxy.y][cellxy.x].alloc = alloc;

      // move to the next cell coords
      cellxy.x++;
      if (cellxy.x >= CELLS_PER_ROW) {
        cellxy.x = 0;
        cellxy.y++;
      }
    }

    if (cell){
      cell.classList.add("log_area_used_end");
    }
  }

  function hideAllocation(alloc) {
    if (alloc){
      var cellxy = getCellXYByAddr(alloc.addr);
      var addr;
      var cell;

      for (addr = alloc.addr; addr < alloc.addr + alloc.size; addr += BYTES_PER_CELL) {
        // get cell at current coords
        cell = tbl.rows[cellxy.y].cells[cellxy.x];

        // apply appropriate styles
        resetCell(cell);

        // reset celldata
        cellData[cellxy.y][cellxy.x].alloc = undefined;

        // move to the next cell coords
        cellxy.x++;
        if (cellxy.x >= CELLS_PER_ROW) {
          cellxy.x = 0;
          cellxy.y++;
        }
      }
    }
  }

  function createLogger(heapStart, heapEnd, logItems, statTotal) {
    var curItemIdx = 0;
    var allocMap = createAllocMap(heapStart, heapEnd, {
      calcStat: false,
      checkAddresses: DEBUG,
    });
    var i;

    return {
      getItemIdx: function() {
        return curItemIdx;
      },


      getAllocMap: function () {
        return allocMap;
      },

      getItemsCnt: function () {
        return logItems.length;
      },
      getItem: function (num) {
        return logItems[num];
      },
      getCurItem: function() {
        return logItems[curItemIdx];
      },
      getHeapStart: function () {
        return heapStart;
      },
      getHeapEnd: function () {
        return heapEnd;
      },
      getHeapSize: function () {
        return heapEnd - heapStart;
      },

      getStatTotal: function () {
        return statTotal;
      },

      setItemIdx: function (newItemIdx) {
        while (newItemIdx > curItemIdx) {
          forward();
        }

        while (newItemIdx < curItemIdx) {
          backward();
        }
      },
    };

    function forward() {
      curItemIdx++;
      var item = logItems[curItemIdx];

      if (item.type == ItemType.MALLOC) {
        guiDeltaItems.push(
          allocMap.malloc(item.addr, item.size, item.shim, item)
        );
      } else if (item.type == ItemType.REALLOC) {
        // `item` given to `free` is needed for statistics only, and since
        // the `free` is an intermediary step during realloc, we should pass
        // `undefined` here.
        guiDeltaItems.push(allocMap.free(item.addr_old, undefined));
        guiDeltaItems.push(
          allocMap.malloc(item.addr_new, item.size_new, item.shim_new, item)
        );
      } else if (item.type == ItemType.FREE) {
        guiDeltaItems.push(allocMap.free(item.addr, item));
      } else if (item.type == ItemType.NONE) {
        /* do nothing */
      } else {
        throw Error("wrong item type");
      }
    }

    function backward() {
      var item = logItems[curItemIdx];

      if (item.type == ItemType.MALLOC) {
        guiDeltaItems.push(
          allocMap.free(item.addr, item)
        );
      } else if (item.type == ItemType.REALLOC) {
        // `item` given to `free` is needed for statistics only, and since
        // the `free` is an intermediary step during realloc, we should pass
        // `undefined` here.
        guiDeltaItems.push(allocMap.free(item.addr_new, undefined));
        guiDeltaItems.push(
          allocMap.malloc(item.addr_old, item.size_old, item.shim_old, item.itemWhichAllocated)
        );
      } else if (item.type == ItemType.FREE) {
        guiDeltaItems.push(
          allocMap.malloc(item.addr, item.size, item.shim, item.itemWhichAllocated)
        );
      } else if (item.type == ItemType.NONE) {
        /* do nothing */
      } else {
        throw Error("wrong item type");
      }

      curItemIdx--;
    }
  }

  function parseLogLines(lines) {

    var emptyArray = [];
    var allocMap = createAllocMap(0, 0, {
      calcStat: true,
      checkAddresses: true,
    });
    var logItems = [];
    var heapStart = 0;
    var heapEnd = 0;
    var curStat = new StatLocal();

    var i;
    var hlog_regexp = new RegExp('hl\{(m|c|z|r|f)\,([a-zA-Z0-9_,]+)\}');
    var hlog_param_regexp = new RegExp('hlog_param\:(\{[^}]+\})');
    var hlog_calls_regexp = new RegExp('hcs\{([a-zA-Z0-9_ ]*)\}');
    var curCallsArray = emptyArray;

    /*
     * Return call stack for an item, prepended by a comma (to be used in
     * `toString()`)
     */
    var callStack = function(item) {
      if (item.calls.length > 0) {
        return ", calls: " + item.calls.join(" ← ");
      } else {
        return "";
      }
    }

    /* push initial item */
    var item = new LogItemNone("--- init ---");
    item.stat = curStat;
    logItems.push(item);

    LogItemNone.prototype = {
      toString: function() {
        return "#" + this.idx + " " + this.comment;
      },
    };

    LogItemMalloc.prototype = {
      toString: function() {
        return "#" + this.idx + " " +
          "Alloc: 0x" + this.addr.toString(16) + ", size: " + this.size +
          ", shim: " + this.shim
          + callStack(this)
        ;
      },
    };

    LogItemRealloc.prototype = {
      toString: function() {
        return "#" + this.idx + " "
        + "Realloc: "
        + "0x" + this.addr_old.toString(16) + ", size: " + this.size_old + " "
        + (this.itemWhichAllocated
          ? "(by #" + this.itemWhichAllocated.idx + ")"
          : "")
        + " -> "
        + "0x" + this.addr_new.toString(16) + ", size: " + this.size_new + ", "
        + "shim: " + this.shim_new
        + callStack(this)
        ;
      },
    };

    LogItemFree.prototype = {
      toString: function() {
        return "#" + this.idx + " "
        + "Free: 0x" + this.addr.toString(16) + ", size: " + this.size + ", "
        + "shim: " + this.shim
        + (this.itemWhichAllocated
          ? " (by #" + this.itemWhichAllocated.idx + ")"
          : "")
        + callStack(this)
        ;
      },
    };

    // parse all lines from device log
    for (i = 0; i < lines.length; i++) {
      parseLine(lines[i]);
    }

    // set idx for each item
    for (i = 0; i < logItems.length; i++) {
      logItems[i].idx = i;
    }

    return createLogger(heapStart, heapEnd, logItems, allocMap.getStatTotal());



    function LogItemNone(comment) {
      this.type = ItemType.NONE;
      this.comment = comment;
    }

    function LogItemMalloc(addr, size, shim) {
      this.type = ItemType.MALLOC;
      this.addr = addr;
      this.size = size;
      this.shim = shim;
    }

    function LogItemRealloc(
      addr_old, size_old, shim_old, itemWhichAllocated,
      addr_new, size_new, shim_new
    )
    {
      this.type = ItemType.REALLOC;

      this.addr_old = addr_old;
      this.size_old = size_old;
      this.shim_old = shim_old;

      this.itemWhichAllocated = itemWhichAllocated;

      this.addr_new = addr_new;
      this.size_new = size_new;
      this.shim_new = shim_new;
    }

    function LogItemFree(addr, size, shim, itemWhichAllocated) {
      this.type = ItemType.FREE;
      this.addr = addr;
      this.size = size;
      this.shim = shim;
      this.itemWhichAllocated = itemWhichAllocated;
    }

    function parseLine(line) {
      var item = new LogItemNone();

      var match = line.match(hlog_regexp);
      var callsMatch = line.match(hlog_calls_regexp);
      if (match) {
        /* heap log item */

        var verb = match[1];
        var dataArr = match[2].split(',');
        var delta;

        if (verb == 'm' || verb == 'c' || verb == 'z') {
          /* malloc, calloc, zalloc */
          var data = {
            size: parseInt(dataArr[0]),
            shim: parseInt(dataArr[1]),
            ptr: parseInt(dataArr[2], 16),
          };
          item = new LogItemMalloc(data.ptr, data.size, data.shim);
          delta = allocMap.malloc(data.ptr, data.size, data.shim, item);
        } else if (verb == 'r') {
          /* realloc */
          var data = {
            size: parseInt(dataArr[0]),
            shim: parseInt(dataArr[1]),
            ptr_old: parseInt(dataArr[2], 16),
            ptr: parseInt(dataArr[3], 16),
          };
          var oldAlloc = allocMap.getAllocByAddr(data.ptr_old);
          item = new LogItemRealloc(
            data.ptr_old,
            oldAlloc.size,
            oldAlloc.shim,
            oldAlloc.item,
            data.ptr, data.size, data.shim
          );

          /*
           * `item` will be given to the malloc below, here we pass `undefined`
           */
          allocMap.free(data.ptr_old, undefined);
          delta = allocMap.malloc(data.ptr, data.size, data.shim, item);

        } else if (verb == 'f') {
          /* free */
          var data = {
            ptr: parseInt(dataArr[0], 16),
            shim: parseInt(dataArr[1]),
          };
          var alloc = allocMap.getAllocByAddr(data.ptr);
          item = new LogItemFree(
            data.ptr,
            alloc.size,
            alloc.shim,
            alloc.item
          );
          delta = allocMap.free(data.ptr, item);
        } else {
          throw Error("wrong heaplog verb: " + verb);
        }

        curStat = delta.stat;

        /* If user text is not empty on this line, add separate item for it */
        var user_text = line.replace(hlog_regexp, '').trim();
        if (user_text) {
          var item2 = new LogItemNone();
          item2.comment = user_text;
          item2.stat = JSON.parse(JSON.stringify(curStat));
          logItems.push(item2);
        }

        item.comment = "";
      } else if (line.match(hlog_param_regexp)){
        /* Heap params */
        match = line.match(hlog_param_regexp)

        /* TODO: JSON.parse */
        var inputData = eval("(" + match[1] + ")");

        heapStart = inputData.heap_start;
        heapEnd = inputData.heap_end;

        allocMap.setHeapStartEnd(heapStart, heapEnd);

        item.comment = line;
      } else if (callsMatch){
        /* remember call stack, it will be set to the next heaplog item */
        curCallsArray = callsMatch[1].split(' ');
        curCallsArray = curCallsArray.filter(function(n){ return n != ''; });

        /* prevent adding current item to the array */
        item = undefined;
      } else {
        item.comment = line;
      }

      if (item) {
        item.stat = JSON.parse(JSON.stringify(curStat));
        item.calls = curCallsArray;
        curCallsArray = emptyArray;

        logItems.push(item);
      }
    }
  }

  function readLogFile(e) {
    var file = e.target.files[0];
    if (!file) {
      return;
    }
    var reader = new FileReader();
    reader.onload = function(e) {
      var contents = e.target.result;
      var lines = contents.split(/\n/);

      logger = parseLogLines(lines);
      logger.setItemIdx( logger.getItemsCnt() - 1 );

      guiRebuild();
    };
    reader.readAsText(file);
  }

  function onLogItemChanged(e) {
    var itemIdx = jQuery("#log_sel")[0].selectedIndex;
    setLogItemIdx(itemIdx);
  }

  function setLogItemIdx(itemIdx) {
    logger.setItemIdx(itemIdx);
    guiLoggerStateApply();
  }

  function StatTotal() {
    this.maxHeapUsage = {
      value: 0,
      items: [],
    };
    this.maxHeapUsageWOvh = {
      value: 0,
      items: [],
    };
    this.maxAllocCnt = {
      value: 0,
      items: [],
    };
    this.minChunkSize = {
      value: 0,
      items: [],
    };
  }

  function StatLocal() {
    this.heapUsage = 0;
    this.heapUsageWOvh = 0;
    this.allocCnt = 0;
    this.maxChunkSize = 0;
  }

  function createAllocMap (_heapStart, _heapEnd, param) {
    var curAllocs = [];
    var heapStart, heapEnd, heapSize;

    var statTotal = new StatTotal();

    setHeapStartEnd(_heapStart, _heapEnd);

    var statCur = new StatLocal();

    return {

      /*
       * - `addr`, `size`: allocated buffer
       * - `shim`: whether the allocation goes through Cesanta's shim
       * - `item`: item which caused allocation to exist. It will be used by
       *   the GUI to display relevant information when user interacts with
       *   the map, or whatever. If `param.calcStat` is `true`, `item` is also
       *   used for the statistics calculation.
       */
      malloc: function(addr, size, shim, item) {
        var ret = {
          action: AllocAction.ALLOC,
          alloc: {
            addr: addr,
            size: size,
            shim: shim,
            item: item  // item that caused allocation to exist
          },
        };

        // make sure the memory area determined by `addr` and `size` is free
        if (param.checkAddresses) {
          checkFree(addr, size);
        }

        // remember new allocated buffer
        curAllocs[addr] = ret.alloc;

        // if needed, calculate statistics
        if (param.calcStat) {
          statCur.heapUsage += size;
          statCur.heapUsageWOvh += (size + OVERHEAD_BYTES_PER_ALLOC);
          if (size != 0) {
            statCur.allocCnt++;
            applyStat(statCur, item);
          }

          ret.stat = JSON.parse(JSON.stringify(statCur));
        }

        return ret;
      },

      /*
       * - `addr`: address to free
       * - `item`: item which caused freeing. It is used if only
       *   `param.calcStat` is `true`: it is used for the statistics
       *   calculation.
       */
      free: function(addr, item) {
        var ret = {
          action: AllocAction.FREE,
          alloc: curAllocs[addr],
        };

        // make sure there is known buffer allocated at the given address
        if (param.checkAddresses) {
          checkAllocated(addr);
        }

        var size = getCurAlloc(addr).size;

        // if needed, calculate statistics
        if (param.calcStat) {
          statCur.heapUsage -= size;
          statCur.heapUsageWOvh -= (size + OVERHEAD_BYTES_PER_ALLOC);
          if (size != 0) {
            statCur.allocCnt--;
            applyStat(statCur, item);
          }

          ret.stat = JSON.parse(JSON.stringify(statCur));
        }

        // delete allocated buffer
        delete curAllocs[addr];

        return ret;
      },

      getAllocByAddr: function(addr) {
        if (param.checkAddresses) {
          checkAllocated(addr);
        }
        return getCurAlloc(addr);
      },

      getAllocAddresses: getAllocAddresses,

      setHeapStartEnd: setHeapStartEnd,

      getStatTotal: function() {
        return statTotal;
      },

    };

    function getAllocAddresses() {
      var ret = [];
      var key;
      for (key in curAllocs){
        if (curAllocs.hasOwnProperty(key)) {
          ret.push(key * 1);
        }
      }
      return ret;
    }

    function setHeapStartEnd(_heapStart, _heapEnd) {
      heapStart = _heapStart;
      heapEnd = _heapEnd;

      heapSize = heapEnd - heapStart;
      statTotal.minChunkSize.value = heapSize;
    }

    function calcMaxChunkSize() {
      var addrs = getAllocAddresses().sort();
      var freeStart, freeEnd;
      var i;

      var curMaxChunkSize = 0;
      var curSize;

      freeStart = heapStart;

      for (i = 0; i < addrs.length; i++) {
        freeEnd = addrs[i];
        var alloc = getCurAlloc(addrs[i]);

        curSize = freeEnd - freeStart;

        freeStart = alloc.addr + alloc.size;

        if (curMaxChunkSize < curSize) {
          curMaxChunkSize = curSize;
        }
      }

      curSize = heapEnd - freeStart;
      if (curMaxChunkSize < curSize) {
        curMaxChunkSize = curSize;
      }

      return curMaxChunkSize;
    }

    function applyStat(statCur, item) {
      /*
       * `item` might be `undefined` here in case this particular allocation
       * step represents some intermediary state: for example, realloc is
       * represented by a `free` followed by `malloc`. So we should apply
       * statistics for the latest `malloc`, not for `free`.
       *
       * Therefore, we don't do anything here if `item` is undefined.
       */
      if (item) {
        statCur.maxChunkSize = calcMaxChunkSize();

        if (statTotal.maxHeapUsage.value <= statCur.heapUsage) {
          if (statTotal.maxHeapUsage.value < statCur.heapUsage) {
            statTotal.maxHeapUsage.items.length = 0;
          }
          statTotal.maxHeapUsage.value = statCur.heapUsage;
          statTotal.maxHeapUsage.items.push(item);
        }

        if (statTotal.maxHeapUsageWOvh.value <= statCur.heapUsageWOvh) {
          if (statTotal.maxHeapUsageWOvh.value < statCur.heapUsageWOvh) {
            statTotal.maxHeapUsageWOvh.items.length = 0;
          }
          statTotal.maxHeapUsageWOvh.value = statCur.heapUsageWOvh;
          statTotal.maxHeapUsageWOvh.items.push(item);
        }

        if (statTotal.maxAllocCnt.value <= statCur.allocCnt) {
          if (statTotal.maxAllocCnt.value < statCur.allocCnt) {
            statTotal.maxAllocCnt.items.length = 0;
          }
          statTotal.maxAllocCnt.value = statCur.allocCnt;
          statTotal.maxAllocCnt.items.push(item);
        }

        if (statTotal.minChunkSize.value >= statCur.maxChunkSize) {
          if (statTotal.minChunkSize.value > statCur.maxChunkSize) {
            statTotal.minChunkSize.items.length = 0;
          }
          statTotal.minChunkSize.value = statCur.maxChunkSize;
          statTotal.minChunkSize.items.push(item);
        }
      }
    }

    function checkAllocated(addr) {
      if (!getCurAlloc(addr)) {
        throw Error("no known allocation at addr " + addr.toString(16));
      }
    }

    function checkFree(addr, size) {
      var key;

      // check if new allocation intersects any existing one
      for (key in curAllocs) {
        if (curAllocs.hasOwnProperty(key)
            && curAllocs[key].addr < (addr + size)
            && (curAllocs[key].addr + curAllocs[key].size) >= addr) {
              throw Error(
                "new allocation {" + addr + ", " + size + "} "
                + "intersects with the existing one "
                + "{" + curAllocs[key].addr + ", " + curAllocs[key].size + "}"
              );
            }
      }

      //-- check if new allocation is out of the heap boundaries
      if (addr != 0 && size != 0) {
        if ((addr + size) > heapEnd) {
          throw Error(
            "new allocation {" + addr.toString(16) + ", " + size + "} "
            + " is beyond the end of the heap " + heapEnd.toString(16)
          );
        } else if (addr < heapStart) {
          throw Error(
            "new allocation {" + addr.toString(16) + ", " + size + "} "
            + " is before the beginning of the heap " + heapStart.toString(16)
          );
        }
      }
    }

    function getCurAlloc(addr) {
      if (addr == 0) {
        return {
          addr: 0,
          size: 0
        };
      } else {
        return curAllocs[addr];
      }
    }
  }


})();

