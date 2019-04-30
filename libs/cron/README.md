# A Cron Mongoose OS library

## Overview

This library implement cron core functionality. See also:
[crontab](https://github.com/cesanta/mongoose-os/libs/crontab).

This library supports classic cron expressions with additional field for the
seconds, plus a few extras. We'll get back to the extras later, let's start
with regular cron expressions.

### Classic cron expressions.

Each expression contains 6 fields:

  - seconds, `0-59`
  - minutes, `0-59`
  - hours, `0-23`
  - day of month, `1-31`
  - month, `1-12` or `JAN-DEC`
  - day of week `0-6` or `SUN-SAT`

Each field can contain multiple values separated with commas, and/or ranges
determined by the beginning of the range, a hyphen, and the ending of the
range. For example, for the day of week, it could be `MON-THU,SAT`

A slash can be used to specify intervals: e.g. `*/5` in seconds field means
"every 5 seconds".

Each field can contain an asterisk `*` which means "any value".

Examples:

- `*/15 * 1-4 * * *` : Run every 15 seconds from 1 to 4 hours;
- `0 */2 1-4 * * *` : Run every two minutes from 1 to 4 hours;
- `0 0 7 * * MON-FRI` : Run at 7:00 every working day;
- `0 30 23 30 * *` : Run at 23:30 every 30th day of month.

### Randomized cron expressions

It's possible to specify the time range in which a job will run certain number
of times.

For example, this:

```
@random:{"from":"10 * * * * *", "to":"50 * * * * *", "number":5}
```

means that the cron job will be fired in between of 10 and 50 seconds of every
minute, and the total number of invocations in that 40-second time window will
be approximately 5. It's not guaranteed to be exactly 5 though: might be a bit
less or more.

Another example: approximately 10 invocations in between of 08:00 and 22:00 on
working days:

```
@random:{"from":"0 0 8 * * MON-FRI", "to":"0 0 22 * * MON-FRI", "number":10}
```

Note that the timeout between `from` and `to` should always be the same.
For example, this would make no sense:

```
@random:{"from":"0 0 8 * * MON-FRI", "to":"0 0 22 * * *", "number":10}
```

Currently these cases are not checked yet, so the above expression will just
result in incorrect behavior.

### Sunrise/sunset

Finally, instead of specifying exact time with the first three fields (seconds,
minutes, hours) one could use `@sunrise` / `@sunset`, plus optional offset like
`+2h45m` or `-1.5h`.

For that to work, the device needs to have latitude/longtitude and timezone
configured.

Examples:

- `@sunrise * * MON-FRI` : Run at the sunrise on working days;
- `@sunset-1h30m 1 * *` : Run 1.5 hours before the sunset every 1th day of month;
- `@sunset * * *` : Run at the sunset every day

As a shortcut for the last case, when all fields are `*`, these fields can be
omitted. Thus, more valid examples:

- `@sunrise`
- `@sunset`
- `@sunset+3h`
