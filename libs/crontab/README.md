# Crontab

Crontab wraps [cron core](https://github.com/cesanta/mongoose-os/libs/cron) and
maintains a persisted set of cron jobs. Crontab file is simply a JSON file
(actually managed by [jstore](https://github.com/cesanta/mongoose-os/libs/jstore))
which looks like this:

```javascript
{"items":[
  ["1", {
    "at": "0 0 7 * * MON-FRI",
    "enable": true,
    "action": "foo",
    "payload": {"a": 1, "b": 2}
  }],
  ["2", {
    "at": "0 */2 1-4 * * *",
    "enable": true,
    "action": "bar"
  }]
]}
```

For the cron expression syntax, see [cron
core](https://github.com/cesanta/mongoose-os/libs/cron) docs.
