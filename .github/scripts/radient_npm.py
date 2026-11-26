"""Version and publish the exact Radient tarball validated by Emscripten CI."""
import argparse
import base64
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import urlopen

PACKAGE = "@diligentgraphics/radient"
REGISTRY = "https://registry.npmjs.org"
REPOSITORY = "https://github.com/DiligentGraphics/DiligentEngine"
BUILD_WORKFLOW = ".github/workflows/build-emscripten.yml"
NUMBER = r"(?:0|[1-9][0-9]*)"
BASE = rf"({NUMBER})\.({NUMBER})\.({NUMBER})"
DEV = re.compile(rf"{BASE}-dev\.({NUMBER})(?:\.({NUMBER}))?")


def dev_version(manifest, run, attempt):
    if manifest["name"] != PACKAGE or run < 1 or attempt < 1:
        raise ValueError("Expected the Radient package and positive run/attempt numbers")
    base = manifest["version"].split("-", 1)[0]
    if not re.fullmatch(BASE, base):
        raise ValueError(f"Invalid package base version: {base}")
    return f"{base}-dev.{run}.{attempt}"


def version_key(version):
    match = DEV.fullmatch(version)
    if not match:
        raise ValueError(f"Expected a development version, received {version!r}")
    return tuple(int(part or 0) for part in match.groups())


@dataclass(frozen=True)
class Package:
    path: Path
    version: str
    integrity: str
    built_attempt: int


def select_artifact(root, run, attempt):
    if run < 1 or attempt < 1:
        raise ValueError("Expected positive build run and attempt numbers")
    # Failed-job reruns can reuse a package produced by an earlier attempt.
    # Keep attempt directories separate; never merge different tested tarballs.
    pattern = re.compile(rf"RadientNpmPackage-{run}-([1-9][0-9]*)")
    candidates = []
    for path in root.iterdir():
        match = pattern.fullmatch(path.name)
        if path.is_dir() and match and int(match[1]) <= attempt:
            candidates.append((int(match[1]), path))
    if not candidates:
        raise ValueError("No package artifact was found for this workflow run")
    return max(candidates)


def package_subject(package):
    # npm requires a package URL and SHA-512, rather than a filename and SHA-256.
    digest = base64.b64decode(package.integrity.removeprefix("sha512-"), validate=True).hex()
    return {"name": f"pkg:npm/{quote(PACKAGE, safe='/')}@{package.version}", "digest": {"sha512": digest}}


def check_provenance(path, package, commit, run_id, signing_attempt):
    if run_id < 1 or signing_attempt < package.built_attempt:
        raise ValueError("Invalid source signing run or attempt")
    # actions/attest generates provenance from the source job's OIDC claims.
    # npm verifies its certificate, signature, and transparency log at publication.
    bundle = json.loads(path.read_text())
    envelope = bundle["dsseEnvelope"]
    if envelope["payloadType"] != "application/vnd.in-toto+json":
        raise ValueError("Unexpected provenance payload type")
    statement = json.loads(base64.b64decode(envelope["payload"], validate=True))
    if (statement.get("_type") != "https://in-toto.io/Statement/v1" or
            statement.get("predicateType") != "https://slsa.dev/provenance/v1"):
        raise ValueError("Expected SLSA v1 build provenance")
    if statement.get("subject") != [package_subject(package)]:
        raise ValueError("Provenance does not identify the tested package and SHA-512")

    predicate = statement["predicate"]
    definition = predicate.get("buildDefinition", {})
    workflow = {"repository": REPOSITORY, "path": BUILD_WORKFLOW, "ref": "refs/heads/master"}
    dependency = {"uri": f"git+{REPOSITORY}@refs/heads/master", "digest": {"gitCommit": commit}}
    details = predicate.get("runDetails", {})
    # A failed-job retry can sign an earlier package. Its invocation must match
    # the signing attempt, while the package version retains its producing attempt.
    invocation = f"{REPOSITORY}/actions/runs/{run_id}/attempts/{signing_attempt}"
    if (definition.get("buildType") != "https://actions.github.io/buildtypes/workflow/v1" or
            definition.get("externalParameters", {}).get("workflow") != workflow or
            definition.get("resolvedDependencies") != [dependency] or
            details.get("builder", {}).get("id") != f"{REPOSITORY}/{BUILD_WORKFLOW}@refs/heads/master" or
            details.get("metadata", {}).get("invocationId") != invocation):
        raise ValueError("Provenance does not belong to the source build, attempt, and commit")

    github = definition.get("internalParameters", {}).get("github", {})
    identifiers = (github.get("repository_id"), github.get("repository_owner_id"))
    if (github.get("event_name") != "push" or
            any(not isinstance(value, str) or not re.fullmatch(r"[1-9][0-9]*", value)
                for value in identifiers)):
        raise ValueError("Provenance is missing valid source GitHub event or repository identifiers")


def provenance_path(root, run, attempt):
    # Require the bundle from the completed source attempt. A stale or future
    # signing artifact must not hide a failed signing job in this attempt.
    path = root / f"RadientNpmPackage-{run}-{attempt}-provenance" / "attestation.json"
    if not path.is_file():
        raise ValueError("Missing provenance for this source build attempt; rerun the source "
                         "Emscripten provenance job before retrying publication")
    return path


def read_package(root, run, attempt, commit):
    built_attempt, directory = select_artifact(root, run, attempt)
    files = list(directory.glob("*.tgz"))
    if len(files) != 1:
        raise ValueError("Expected exactly one tested tarball in the selected artifact")
    path = files[0]
    checksum = (directory / "package.sha256").read_text().strip().split()
    data = path.read_bytes()
    if checksum != [hashlib.sha256(data).hexdigest(), path.name]:
        raise ValueError("Package SHA-256 or filename does not match the tested artifact")
    # Inspect metadata in place: never extract, modify, or repack the package.
    with tarfile.open(path, "r:gz") as archive:
        manifest = json.load(archive.extractfile("package/package.json"))
        info = json.load(archive.extractfile("package/build-info.json"))
    expected = dev_version(manifest, run, built_attempt)
    if manifest["version"] != expected:
        raise ValueError(f"Expected tested package version {expected}, received {manifest['version']}")
    if info.get("engine", {}).get("commit") != commit:
        raise ValueError("Package does not belong to this main-repository commit")
    integrity = "sha512-" + base64.b64encode(hashlib.sha512(data).digest()).decode("ascii")
    return Package(path.resolve(), expected, integrity, built_attempt)


def registry_metadata(version):
    url = f"{REGISTRY}/{quote(PACKAGE, safe='')}/{quote(version, safe='')}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.load(response)
    except HTTPError as error:
        if error.code == 404:
            return None
        raise


def run_npm(*arguments):
    npm = shutil.which("npm.cmd") or shutil.which("npm")
    if not npm:
        raise RuntimeError("npm is required for publishing")
    subprocess.run([npm, *map(str, arguments), f"--registry={REGISTRY}"], check=True)


def publish(package, provenance):
    current = registry_metadata("dev")
    advance = current is None or version_key(package.version) > version_key(current["version"])
    existing = registry_metadata(package.version)
    if existing is not None and existing.get("dist", {}).get("integrity") != package.integrity:
        raise ValueError("This npm version already exists with different package contents")

    if existing is None:
        # A slow older build must still publish, but must not move dev backward.
        tag = "dev" if advance else "dev-staging"
        run_npm("publish", package.path, "--access=public", f"--tag={tag}",
                "--ignore-scripts", f"--provenance-file={provenance}")
    elif advance:
        # Recover if a previous publish succeeded but updating the tag did not.
        run_npm("dist-tag", "add", f"{PACKAGE}@{package.version}", "dev")
    else:
        print(f"Already published {PACKAGE}@{package.version}; dev remains unchanged")

    if not advance:
        staging = registry_metadata("dev-staging")
        if staging and staging["version"] == package.version:
            run_npm("dist-tag", "rm", PACKAGE, "dev-staging")
    print(f"Published {PACKAGE}@{package.version}; " + ("advanced dev" if advance else "left dev unchanged"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    version = commands.add_parser("version")
    version.add_argument("--manifest", type=Path, required=True)
    prepare = commands.add_parser("prepare", help="Verify the package and emit its attestation subject")
    check = commands.add_parser("publish")
    check.add_argument("--run-id", type=int, required=True)
    check.add_argument("--verify-only", action="store_true",
                       help="Check local artifact and provenance metadata; no signature or registry verification")
    for command in (prepare, check):
        command.add_argument("--artifacts", type=Path, required=True)
        command.add_argument("--commit", required=True)
    for command in (version, prepare, check):
        command.add_argument("--run", type=int, required=True)
        command.add_argument("--attempt", type=int, required=True)
    args = parser.parse_args()
    if args.command == "version":
        print(dev_version(json.loads(args.manifest.read_text()), args.run, args.attempt))
        return
    if args.command == "publish" and args.run_id < 1:
        parser.error("--run-id must be positive")
    package = read_package(args.artifacts, args.run, args.attempt, args.commit)
    if args.command == "prepare":
        subject = package_subject(package)
        print(f"name={subject['name']}")
        print(f"digest=sha512:{subject['digest']['sha512']}")
        return
    provenance = provenance_path(args.artifacts, args.run, args.attempt)
    check_provenance(provenance, package, args.commit, args.run_id, args.attempt)
    print(f"Verified {PACKAGE}@{package.version}: {package.path}", flush=True)
    if not args.verify_only:
        publish(package, provenance.resolve())



if __name__ == "__main__":
    main()
