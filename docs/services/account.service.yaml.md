---
title: "Account"
---

Provides methods for managing users. Used by the frontend.

#### CreateUser
Creates a new user. Can only be called by the frontend.

Arguments:
- `password`: Password for the user.
- `email`: User's email address.
- `name`: Display name for the user.
- `id`: ID for the new user.

#### GetInfo
Retrieves info about an existing user.

Arguments:
- `id`: ID of the user.

Result `object`: 

