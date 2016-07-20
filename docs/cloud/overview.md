---
title: Overview
---

Mongoose IoT Platform is organised in the following way:

- Accounts
   - Users. Indivual users can be authenticated
   - Organisations. A way to group users.
- Devices
- Projects
   - Source code
   - Repository
   - Per user-working copy (for the IDE)
   - Built firmware
   - Deployed and preview webapps mounted on hostnames
   - All devices that are running FW built from this project

When the user accesses Mongoose Cloud for the first time, we create a user
account for her. This account identifies her and her only. A user can also
create organisations. User and organisations are accounts.

A device can be owned by only one account at a given time.
The ownership of a device can be transferred from one account to another.

A user can create a project. A project belongs to an account and one account
only. It can thus belong to either a user or to an organisation.

Each project logically contains a code repository that hosts the sources
for the various apps, such as device firmware, webapp(s), mobile app(s).
That repository contains all the code necessary to build a project.

Mongoose Cloud offers an IDE that allows the user to work on code stored
in the repository. The IDE provides each user with a working copy of a
repository.

The code repository and thus the working copy, contain a bunch of files and
directories. The IDE editor edits code in a working copy.
The IDE will provide a way to push the changes back into the repository.

The webapp preview is served live from the working copy used by the IDE.
The frontend serves deployed webapps also from a working copy.

Firmware is built from code living in a working copy.

The built firmware metadata mentions the project ID. Devices running that FW
will report it to the cloud. The UI can thus show all devices that implement
that project.

Authorisation is performed at the level of accounts (users or organisations).
A device can switch projects merely by being reflashed with a FW belonging to
another project.

Projects can be published and appear in a app stores.
Other users can fork those projects, like they are used to do on GitHub.

When a new device is onboarded to the cloud via
[Mongoose Flashing Tool](https://github.com/cesanta/mft), it is assigned
an ID and a password (also called a Pre-Shared Key, PSK). The device's ID is
globally unique.
