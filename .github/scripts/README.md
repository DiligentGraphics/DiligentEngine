# Radient npm development publishing

## Publishing behavior

[Emscripten build](../workflows/build-emscripten.yml) assembles and tests the npm
package. The separate [Publish npm package](../workflows/publish-npm.yml) workflow
publishes only after the full Emscripten matrix succeeds for a push to
`DiligentGraphics/DiligentEngine`'s `master` branch. Pull requests and other
branches do not publish. Native platform workflows are not publication prerequisites.

The package version is `<base>-dev.<build-run-number>.<build-attempt>`, using the
base version from the SDK's `package.json`. It is assigned before assembly and
consumer testing. Publication verifies and uses the exact tested tarball without
rebuilding or repacking it.

Newer versions advance the `dev` tag; older builds can publish without moving it
backward. `latest` is unchanged. Only development publishing is currently supported.
Install development builds with `npm install @diligentgraphics/radient@dev`.

After the build matrix succeeds, **Sign Radient npm provenance** signs the tested
package in the same Emscripten workflow. This keeps the provenance's workflow,
commit, and run identity consistent with the signing certificate, as npm requires.
The separate publisher verifies and submits that signed bundle with the tarball.

The signing job is allowed to fail without failing the Emscripten workflow, so
signing outages do not block Web demo deployment. npm publication still requires
the signed bundle; registry failures affect only the publishing workflow.

## Failure recovery

- Retry the failed **Publish npm package** workflow to publish the same tested
  package and version. Retries accept an already-published version only when its
  contents match, and can repair an incomplete tag update.
- If provenance is missing or signing failed, rerun **Sign Radient npm provenance**
  in the originating Emscripten run. This reuses the tested package and triggers
  a new publisher when the run completes. Retrying the old publisher keeps its
  original source attempt and will not use a bundle from the newer attempt.
- If the package or provenance artifact has expired, rerun the Emscripten build. A full build
  rerun creates a new attempt-specific version and artifact. Retrying only failed
  build jobs may reuse a package from an earlier successful packaging attempt.
- Workflow fixes require a new push/build: rerunning an old run uses its original
  workflow revision.
- For authentication failures, check [Initial npm setup](#initial-npm-setup)
  before retrying publication.
- If publication never starts, confirm the source build was an eligible,
  successful `master` push and `publish-npm.yml` exists on the default branch.

## Local validation

Run these commands from the repository root. The helper tests use temporary
packages and mocked registry calls; they do not publish anything. CI runs them
before packaging and again before signing:

```sh
python3 -B -m unittest discover -s .github/scripts/tests -v
```

To verify a downloaded package and print its signing inputs, use the originating
Emscripten build's run number, attempt, and commit. Replace the placeholders
with your source build's values in both commands and directory names.
The `<build-run-id>` placeholder is the separate numeric ID from the
GitHub Actions run URL; it is not the run number.

```sh
python3 -B .github/scripts/radient_npm.py prepare \
  --artifacts /path/to/artifacts --run <run-number> --attempt <attempt> \
  --commit <main-repository-sha>
```

The artifacts directory must contain
`RadientNpmPackage-<run-number>-<attempt>/`, with the tarball and `package.sha256`.
Preparation prints the package URL and digest without
signing or contacting npm.

To check the signed bundle from the source build without publishing:

```sh
python3 -B .github/scripts/radient_npm.py publish \
  --artifacts /path/to/artifacts --run-id <build-run-id> --run <run-number> --attempt <attempt> \
  --commit <main-repository-sha> --verify-only
```

The artifacts directory must also contain
`RadientNpmPackage-<run-number>-<attempt>-provenance/attestation.json`.
Its attempt identifies the
signing job invocation; the selected package can come from an earlier attempt.

These checks validate package and provenance metadata. They do not verify live
authentication or cryptographic signatures; npm verifies the signature during
actual publication.

## Initial npm setup

Use an npm account authorized to publish `@diligentgraphics/radient`. If the package
does not exist, publish an actual tested tarball with that account before
configuring its GitHub trusted publisher:

- Repository owner: `DiligentGraphics`
- Repository: `DiligentEngine`
- Workflow file: `publish-npm.yml`
- Environment: leave empty
- Allowed operations: direct `npm publish` and `npm dist-tag`

No npm token secret is required. Complete the first successful trusted publication
within two days of creating the connection. If the workflow filename changes,
delete and recreate the npm connection with the new filename.

See [npm trusted publishing](https://docs.npmjs.com/trusted-publishers/) for setup
instructions. This configuration is required before automated publication can succeed.
