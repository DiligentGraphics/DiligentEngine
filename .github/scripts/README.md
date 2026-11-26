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

Provenance identifies the original build and source commit; the publishing
workflow signs it. Signing and npm failures affect only the publishing workflow
and do not prevent the website from deploying successful Emscripten demo artifacts.

## Failure recovery

- Retry the failed **Publish npm package** workflow to publish the same tested
  package and version. Retries accept an already-published version only when its
  contents match, and can repair an incomplete tag update.
- If the package artifact has expired, rerun the Emscripten build. A full build
  rerun creates a new attempt-specific version and artifact. Retrying only failed
  build jobs may reuse a package from an earlier successful packaging attempt.
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

To verify a downloaded package and prepare its unsigned provenance, use the
originating Emscripten build's run ID, run number, attempt, and commit:

```sh
python3 -B .github/scripts/radient_npm.py prepare \
  --artifacts /path/to/artifacts --run-id <build-run-id> --run 482 --attempt 1 \
  --commit <main-repository-sha> --predicate-path /path/to/predicate.json
```

The artifacts directory must contain `RadientNpmPackage-482-1/`, with the tarball
and `package.sha256`. Preparation writes the unsigned predicate and prints the
signing inputs without contacting npm.

To check the signed bundle returned by the publishing workflow without publishing:

```sh
python3 -B .github/scripts/radient_npm.py publish \
  --artifacts /path/to/artifacts --run-id <build-run-id> --run 482 --attempt 1 \
  --commit <main-repository-sha> \
  --provenance /path/to/attestation.json --verify-only
```

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
