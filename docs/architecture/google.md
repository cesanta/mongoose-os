---
title: Google IoT Core integration
---

- First, upload the private key:

```bash
mos put my-device.key.pem
```

- Set project, region, registry and device names as required.

```bash
export PROJECT=my-project
export REGION=us-central1
export REGISTRY=my-registry
export DEVICE=test1-ec
```

- Then, configure the GCP client:

```bash
mos config-set \
  mqtt.enable=true \
  mqtt.server=mqtt.googleapis.com:8883 \
  mqtt.ssl_ca_cert=ca.pem \
  sntp.enable=true \
  gcp.enable=true \
  gcp.project=$PROJECT \
  gcp.region=$REGION \
  gcp.registry=$REGISTRY \
  gcp.device=$DEVICE \
  gcp.key=my-device.key.pem
```

Simplified provisioning via the `mos-gcp-setup` tool is in development.
