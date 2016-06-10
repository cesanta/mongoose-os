---
title: "Timeseries"
---

Metrics service provides timeseries storage.

#### Report
Report adds new value to the storage. Label 'src' is implicitly added.

Arguments:
- `timestamp`: Timestamp.
- `labels`: Label set.
- `value`: Value.

#### Query
Query returns the result of executing `query` on the data stored. `query` uses Prometheus query language.

Arguments:
- `query`: Query to execute.

Result `array`: 
#### ReportMany
ReportMany adds new values to the storage. Label 'src' is implicitly added to each timeseries.

Arguments:
- `timestamp`: Timestamp.
- `vars`: List of labels and values. Each element must be an array with 2 elements: an object, which keys and values are used as label names and label values respectively, and a number - the actual value.


