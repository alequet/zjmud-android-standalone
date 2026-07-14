# zjmud upstream patch

This directory records fixes that must be applied to the public zjmud source
before any Android-specific transformation.

Upstream baseline:

- Repository: <https://github.com/MudRen/zjmud>
- Commit: `c56a166380d74858d7b4f0ba2817478ccea6b83d`
- GitHub source ZIP SHA-256:
  `61ce705fd694bcc3ba4619c4a475e020fe4237df226fb827697de0d682e8b014`

`web_frontend.patch` reproduces the repaired `web/www/main.js` that was used
by the working local zjmud snapshot. It fixes the local login and registration
flow, character-creation controls, ANSI output default, and persisted chat
height. After applying the patch, `web/www/main.js` has SHA-256:

```text
d5ce16fb65a1beb9d39989244b832e4d776ff0db193f75074e8b69dad77b121c
```

Local builds use the already repaired `~/zjmud-main.zip`. The import script
validates both this patch file and the repaired `web/www/main.js` hash, but does
not apply the patch a second time. The public upstream repository is provenance,
not the local build input; it must receive this patch before it can be used to
prepare an equivalent source snapshot.
