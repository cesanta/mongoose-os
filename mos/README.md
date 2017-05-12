The Mongoose OS command line tool
=================================

## Changes

- May 12: manual serial port selection


## Building

In order to build `mos` tool, the following tools are needed:

- [Go](https://golang.org/) v1.8+
- `govendor` tool (install with `go get github.com/kardianos/govendor`)

Make sure you have `GOPATH` set. The `mongoose-os` repository has to be cloned
as `$GOPATH/src/cesanta.com`.

Then, from the repository, invoke:

```
$ govendor sync
```

That will fetch all vendored packages and save them under
`$GOPATH/src/cesanta.com/vendor`.

Now, `mos` tool can be built:

```
$ go build
```
