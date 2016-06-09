---
title: Addressing
---

Address is a subset of URL. Only the host and URI parts are used, the rest
(scheme, user, query and fragment) must be empty.  From the protocol point of
view addresses are composed of 2 pieces: ID and component. They are separated
by "." in the URI part of the address.  Only one component is allowed (i.e. no
more than one dot in the URI part of the address). ID part is used for
authentication and full address is used for routing. This done like that so
there can be multiple processes sharing the same credentials and representing
different aspects of the same entity in the cloud, and at the same time
commands and responses can be routed easily between them.

Addresses are entirely opaque to the routing logic, structure is imposed only
for convenience of authentication and to allow infering address for the
underlying transport if not given explicitly.

In terms of RFC 3986:

```
address = ID ["." component]
ID = "//" host "/" path-rootless
```

One exception: last path segment cannot contain "." (unless it's
percent-encoded). Also note that it can be partially percent-encoded and the
implementation must decode it before comparing with another address.

