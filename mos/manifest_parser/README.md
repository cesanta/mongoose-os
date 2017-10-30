# Manifest parser

Manifest parser takes an app's or lib's manifest, and generates an aggregate
manifest which contains stuff from the app and all libs. Manifest merging is
happening as follows:

  - Combine paths and absolutize them properly: `sources`, `includes`,
    `filesystem`, `binary_libs`;
  - Simply combine other items: `modules`, `libs`, `config_schema`, `cflags`, `cxxflags`;
  - Expand vars in string maps: `build_vars`, `cdefs` (like, if we have a build
    var `FOO` somewhere, we can refer to it as `${build_vars.FOO}`, and even
    extend it like `FOO: "${build_vars.FOO} bar"`);
  - Narrow the set of supported platforms.
  - Combine `conds`: recursively apply the same to conds, prepending paths etc;

Also, each component's manifest can contain `conds`, e.g.

```yaml
conds:
  - when: ${build_vars.FOO} == "1"
    apply:
      build_vars:
        BAR: bar
```

And these conds should be expanded as late as possible: e.g. we could define
`build_vars.FOO` to be `0` in some lib (and thus by default the `BAR` won't be
defined by the cond above), but then the app can override it to `1`, and the
`BAR` should be defined finally.

## Details

Let's consider an example: `app` depends on `libA` which depends on `libB`. For
the further discussion, assume that `libB` has this:

```yaml
build_vars:
  VAR_FROM_LIB_B: from_lib_b
```

Parsing of the app looks as follows:

### Step 1: read all inidividual components' manifests

First of all, we read manifest + arch-specific submanifest of the `app`, *and*
expressions in arch-specific submanifest are expanded against the app's main
manifest:

  - `app -> app_arch = app_whole`

Expressions `${...}` which failed to expand at this point are left unexpanded:
e.g. submanifest could depend on `rootManifest` or on some lib, neither of
which is available at this point, so those expressions will be expanded later,
when we build an aggregate manifest (see below).

So now, `app_whole` contains all libs `app` depends on directly. Now we read
each of those libs (which in our case is just `libA`), and perform the same
parsing recursively:

  - `libA -> libA_arch = libA_whole`

And then again recursively form `libB`:

  - `libB -> libB_arch = libB_whole`

So now we have full manifests for all 3 components, and we build an aggregate
manifest:

### Step 2: compose aggregate manifest

  - `rootManifest -> libB_whole -> libA_whole -> app_whole = aggregate`

So this time, components can actually use stuff from dependencies: `libA` can
have something like:

```yaml
build_vars:
  VAR_FROM_LIB_B: ${build_vars.VAR_FROM_LIB_B} and_from_lib_a
```

which will result in `VAR_FROM_LIB_B` having the value `"from_lib_b and_from_lib_a"`.

When this aggregate manifest is ready, conds come into place.

### Step 3: expand conds, if any

If aggregate manifest contains no conds, we're done and return that aggregate
manifest. Otherwise (there are some conds), cond expressions are evaluated
against the aggregate manifest, but they are expanded into the individual
components' manifests: conds in `libA_whole` expand into `libA_whole`, conds in
`app_whole` expand into `app_whole`, etc. It's done this way because everything
under conds should be present in aggregate manifest in topological order.

When we expand conds for each component, we go back to step 2 and repeat.

### Step 4: add sources or binary libs

Actually, the aggregate manifest at this point doesn't have aggregate sources
from all components. First, we don't want to add all deps' sources when building
a lib, and second, for some lib we might want to add a prebuilt binary instead
of sources.

So at this step, we resolve all libs' sources (by "resolve" I mean that we
convert paths like `/path/to/dir` and globs like `/path/to/dir/*.foo` into
absolute paths to concrete files), and depending on `preferPrebuiltLibs` flag,
whether prebuilt binary exists and whether any source code exists, we add to
the aggregate manifest either sources or prebuilt binary.

That's basically all.

From the description above, we can conclude a few more limitations:

### Limitations

**LIMITATION_1** is as follows:

So in the example above, `libB` defines a build var `VAR_FROM_LIB_B`, so that
"upper" components, like `libA` or `app`, can refer to it with
`${build_vars.VAR_FROM_LIB_B}`.

Now, consider the case when `libB` defines that build var under some cond, like
this:

```yaml
conds:
  - when: ${mos.platform} == "esp32"
    apply:
      build_vars:
        VAR_FROM_LIB_B: from_lib_b
```

Now, if `libA` has this:

```yaml
build_vars:
  VAR_FROM_LIB_B: ${build_vars.VAR_FROM_LIB_B} and_from_lib_a
```

It will complain that it can't evaluate `build_vars.VAR_FROM_LIB_B`. This is
because, while we're building aggregate manifest (`rootManifest -> libB_whole
-> libA_whole .....`), `libB` doesn't yet contain that build var: it's located
in a non-expanded cond. So to make it work, `libA` should override that build
var under a cond as well:

```yaml
conds:
  - when: ${mos.platform} == "esp32"
    apply:
      build_vars:
        VAR_FROM_LIB_B: ${build_vars.VAR_FROM_LIB_B} and_from_lib_a
```

So, to summarize: we can use stuff from dependencies **on the same conds
level**. If some var is defined at the root level (not under any conds), we can
use it in the root level as well (or deeper). If the var is defined in the
first conds level, we should use it minimum in the first conds level (or
deeper). Etc.

And no, we can't just expand conds earlier, because, as was mentioned in the
beginning, one of the goals is to expand conds as late as possible, so that
app can override things these conds depend on.

TODO: probably we can try to apply the same workaround as we do when expanding
arch-specific submanifest: if we can't evaluate the expression, postpone the
evaluation for later, when we might have expanded some conds. However, this
might add more confusion: like, if some lib has two levels of conds:

```yaml
conds:
  - when: ....
    apply:
      build_vars:
        SOME_VAR: 1
      conds:
        - when: ....
          apply:
            build_vars:
              SOME_VAR: ${build_vars.SOME_VAR} 2
```

and in the app we use that `SOME_VAR` on a root level like this:

```yaml
build_vars:
  SOME_VAR: ${build_vars.SOME_VAR} app
```

Then in the end `SOME_VAR` will end up being `1 app 2`, which is confusing. So,
not sure whether it's a good idea in this case.

**LIMITATION_2** is as follows:

Conds can't contain libs. At the time we expand conds, we don't look at libs
anymore. BUT, even if we do, it would make `LIMITATION_1` even more confusing:

Consider that `libA` contains a cond which adds one more library `libC` if the
platform is esp8266. Then, cond levels for that `libC` will be shifted: what is
root level for `libC`, will be the level 1 for all the rest of the components.
Omg.
