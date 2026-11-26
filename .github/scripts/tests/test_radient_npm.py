"""Check publication decisions without contacting npm or publishing packages."""
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import radient_npm as sdk


class VersionTest(unittest.TestCase):
    def test_version_uses_base_run_and_attempt(self):
        for version in ("0.1.0", "0.1.0-dev.0", "0.1.0-dev.20.2"):
            self.assertEqual(sdk.dev_version({"name": sdk.PACKAGE, "version": version}, 42, 3), "0.1.0-dev.42.3")

    def test_invalid_inputs(self):
        for name, version, run, attempt in (("wrong", "0.1.0", 1, 1),
                                            (sdk.PACKAGE, "01.1.0", 1, 1),
                                            (sdk.PACKAGE, "v0.1.0", 1, 1),
                                            (sdk.PACKAGE, "0.1", 1, 1),
                                            (sdk.PACKAGE, "0.1.0+metadata", 1, 1),
                                            (sdk.PACKAGE, "0.1.0", 0, 1),
                                            (sdk.PACKAGE, "0.1.0", 1, 0)):
            with self.subTest(name=name, version=version, run=run, attempt=attempt):
                with self.assertRaises(ValueError):
                    sdk.dev_version({"name": name, "version": version}, run, attempt)

    def test_numeric_version_order(self):
        versions = ["0.1.0-dev.0", "0.1.0-dev.9.1", "0.1.0-dev.10.1",
                    "0.1.0-dev.10.2", "0.2.0-dev.1.1", "0.10.0-dev.1.1"]
        self.assertEqual(sorted(reversed(versions), key=sdk.version_key), versions)
        with self.assertRaises(ValueError):
            sdk.version_key("0.1.0-rc.1")


class ArtifactTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def artifact(self, attempt=1, version=None, commit="commit", name=sdk.PACKAGE):
        directory = self.root / f"RadientNpmPackage-42-{attempt}"
        directory.mkdir()
        path = directory / "sdk.tgz"
        metadata = {"package.json": {"name": name, "version": version or f"0.1.0-dev.42.{attempt}"},
                    "build-info.json": {"engine": {"commit": commit}}}
        with tarfile.open(path, "w:gz") as archive:
            for file, content in metadata.items():
                data = json.dumps(content).encode()
                entry = tarfile.TarInfo(f"package/{file}")
                entry.size = len(data)
                archive.addfile(entry, io.BytesIO(data))
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        (directory / "package.sha256").write_text(f"{digest}  {path.name}\n")
        return path

    def bundle(self, package):
        # Deliberately unsigned fixture: npm performs cryptographic verification
        # of the real GitHub bundle, while these tests check the statement.
        statement = {
            "_type": "https://in-toto.io/Statement/v1",
            "predicateType": "https://slsa.dev/provenance/v1",
            "subject": [{"name": f"pkg:npm/%40diligentgraphics/radient@{package.version}",
                         "digest": {"sha512": hashlib.sha512(package.path.read_bytes()).hexdigest()}}],
            "predicate": {
                "buildDefinition": {
                    "buildType": "https://actions.github.io/buildtypes/workflow/v1",
                    "externalParameters": {"workflow": {
                        "repository": sdk.REPOSITORY, "path": sdk.BUILD_WORKFLOW, "ref": "refs/heads/master"}},
                    "resolvedDependencies": [{"uri": f"git+{sdk.REPOSITORY}@refs/heads/master",
                                              "digest": {"gitCommit": "commit"}}]},
                "runDetails": {
                    "builder": {"id": f"{sdk.REPOSITORY}/{sdk.BUILD_WORKFLOW}@refs/heads/master"},
                    "metadata": {"invocationId":
                        f"{sdk.REPOSITORY}/actions/runs/12345/attempts/{package.built_attempt}"}}}}
        path = self.root / "attestation.json"
        self.write_statement(path, statement)
        return path

    @staticmethod
    def write_statement(path, statement):
        path.write_text(json.dumps({"dsseEnvelope": {"payloadType": "application/vnd.in-toto+json",
            "payload": base64.b64encode(json.dumps(statement).encode()).decode()}}))

    def read(self, attempt=1):
        return sdk.read_package(self.root, 42, attempt, "commit")

    def test_verifies_without_modifying_tarball(self):
        path = self.artifact()
        original = path.read_bytes()
        package = self.read()
        self.assertEqual(package.path, path.resolve())
        self.assertEqual(package.version, "0.1.0-dev.42.1")
        self.assertTrue(package.integrity.startswith("sha512-"))
        self.assertEqual(path.read_bytes(), original)

    def test_failed_job_retry_reuses_tested_attempt(self):
        self.artifact()
        self.assertEqual(self.read(attempt=2).version, "0.1.0-dev.42.1")

    def test_latest_attempt_selected_without_merging_logs(self):
        self.artifact()
        path = self.artifact(attempt=2)
        self.artifact(attempt=3)
        (self.root / "RadientNpmPackage-42-2-logs").mkdir()
        self.assertEqual(self.read(attempt=2).path, path.resolve())

    def test_build_subject_matches_npm_package_url_and_sha512(self):
        path = self.artifact()
        subject = sdk.package_subject(self.read())
        self.assertEqual(subject, {"name": "pkg:npm/%40diligentgraphics/radient@0.1.0-dev.42.1",
                                  "digest": {"sha512": hashlib.sha512(path.read_bytes()).hexdigest()}})

    def test_publisher_commit_does_not_replace_build_commit(self):
        self.artifact()
        with patch.dict(os.environ, {"GITHUB_SHA": "newer-commit", "GITHUB_RUN_ID": "99999",
                                     "GITHUB_RUN_ATTEMPT": "7"}):
            package = self.read(attempt=2)
            predicate = sdk.build_predicate(package, "commit", 12345)
            definition = predicate["buildDefinition"]
            self.assertEqual(definition["resolvedDependencies"][0]["digest"], {"gitCommit": "commit"})
            self.assertEqual(definition["externalParameters"]["workflow"]["path"], sdk.BUILD_WORKFLOW)
            self.assertEqual(predicate["runDetails"]["metadata"]["invocationId"],
                             f"{sdk.REPOSITORY}/actions/runs/12345/attempts/1")
            sdk.check_provenance(self.bundle(package), package, "commit", 12345)

    def test_preparation_needs_no_existing_signed_bundle(self):
        self.artifact()
        package = self.read()
        self.assertEqual(sdk.build_predicate(package, "commit", 12345)["runDetails"]["metadata"]["invocationId"],
                         f"{sdk.REPOSITORY}/actions/runs/12345/attempts/1")
        with self.assertRaises(FileNotFoundError):
            sdk.check_provenance(self.root / "missing.json", package, "commit", 12345)

    def test_prepare_and_verify_cli(self):
        self.artifact()
        package = self.read(attempt=2)
        script = Path(sdk.__file__)
        args = ["--artifacts", str(self.root), "--run", "42", "--attempt", "2",
                "--commit", "commit", "--run-id", "12345"]
        predicate = self.root / "predicate.json"
        result = subprocess.run([sys.executable, "-B", str(script), "prepare", *args,
                                 "--predicate-path", str(predicate)], capture_output=True, text=True, check=True)
        subject = sdk.package_subject(package)
        self.assertEqual(result.stdout.splitlines(), [f"name={subject['name']}",
                                                      f"digest=sha512:{subject['digest']['sha512']}"])
        self.assertEqual(json.loads(predicate.read_text()), sdk.build_predicate(package, "commit", 12345))
        bundle = self.bundle(package)
        subprocess.run([sys.executable, "-B", str(script), "publish", *args,
                        "--provenance", str(bundle), "--verify-only"], capture_output=True, text=True, check=True)
        bundle.unlink()
        result = subprocess.run([sys.executable, "-B", str(script), "publish", *args,
                                 "--provenance", str(bundle), "--verify-only"], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)

    def test_rejects_wrong_provenance_identity(self):
        self.artifact()
        package = self.read()
        path = self.bundle(package)
        envelope = json.loads(path.read_text())["dsseEnvelope"]
        original = base64.b64decode(envelope["payload"])
        changes = [
            (("subject", 0, "name"), "pkg:npm/wrong@0.1.0"),
            (("subject", 0, "digest", "sha512"), "0" * 128),
            (("predicateType",), "https://example.com/not-provenance"),
            (("predicate", "buildDefinition", "externalParameters", "workflow", "path"),
             ".github/workflows/publish-npm.yml"),
            (("predicate", "buildDefinition", "externalParameters", "workflow", "repository"),
             "https://github.com/other/repo"),
            (("predicate", "buildDefinition", "externalParameters", "workflow", "ref"), "refs/heads/other"),
            (("predicate", "buildDefinition", "resolvedDependencies", 0, "digest", "gitCommit"), "newer-commit"),
            (("predicate", "runDetails", "metadata", "invocationId"),
             f"{sdk.REPOSITORY}/actions/runs/99999/attempts/1"),
            (("predicate", "runDetails", "metadata", "invocationId"),
             f"{sdk.REPOSITORY}/actions/runs/12345/attempts/2"),
        ]
        for keys, value in changes:
            with self.subTest(keys=keys, value=value):
                statement = json.loads(original)
                target = statement
                for key in keys[:-1]:
                    target = target[key]
                target[keys[-1]] = value
                self.write_statement(path, statement)
                with self.assertRaises(ValueError):
                    sdk.check_provenance(path, package, "commit", 12345)

    def test_rejects_changed_tarball(self):
        path = self.artifact()
        path.write_bytes(path.read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            self.read()

    def test_does_not_fall_back_from_corrupt_latest_attempt(self):
        self.artifact()
        path = self.artifact(attempt=2)
        path.write_bytes(b"corrupt")
        with self.assertRaises(ValueError):
            self.read(attempt=2)

    def test_rejects_wrong_identity_version_or_commit(self):
        for options in ({"name": "wrong"}, {"version": "0.1.0-dev.41.1"},
                        {"version": "0.1.0-dev.42.2"}, {"commit": "other"}):
            with self.subTest(options=options):
                path = self.artifact(**options)
                with self.assertRaises(ValueError):
                    self.read()
                path.unlink()
                (path.parent / "package.sha256").unlink()
                path.parent.rmdir()

    def test_rejects_invalid_build_numbers(self):
        for run, attempt in ((0, 1), (1, 0), (-1, 1)):
            with self.subTest(run=run, attempt=attempt), self.assertRaisesRegex(ValueError, "positive"):
                sdk.select_artifact(self.root, run, attempt)

    def test_requires_one_tarball(self):
        with self.assertRaises(ValueError):
            self.read()
        path = self.artifact()
        (path.parent / "other.tgz").write_bytes(path.read_bytes())
        with self.assertRaises(ValueError):
            self.read()


class PublishTest(unittest.TestCase):
    def setUp(self):
        self.package = sdk.Package(Path("tested.tgz"), "0.1.0-dev.42.1", "sha512-tested", 1)
        self.provenance = Path("attestation.json")
        self.calls = []
        self.metadata = {}
        self.patch_metadata = patch.object(sdk, "registry_metadata", side_effect=self.metadata.get)
        self.patch_npm = patch.object(sdk, "run_npm", side_effect=lambda *args: self.calls.append(args))
        self.patch_metadata.start()
        self.patch_npm.start()
        self.addCleanup(self.patch_metadata.stop)
        self.addCleanup(self.patch_npm.stop)

    def existing(self, integrity="sha512-tested"):
        self.metadata[self.package.version] = {"version": self.package.version, "dist": {"integrity": integrity}}

    def test_first_publish_uses_exact_tarball_and_dev(self):
        sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [("publish", self.package.path, "--access=public", "--tag=dev",
                                       "--ignore-scripts", f"--provenance-file={self.provenance}")])

    def test_newer_build_advances_dev(self):
        self.metadata["dev"] = {"version": "0.1.0-dev.9.1"}
        sdk.publish(self.package, self.provenance)
        self.assertIn("--tag=dev", self.calls[0])

    def test_older_build_does_not_move_dev_backward(self):
        self.metadata["dev"] = {"version": "0.1.0-dev.43.1"}
        self.metadata["dev-staging"] = {"version": self.package.version}
        sdk.publish(self.package, self.provenance)
        self.assertIn("--tag=dev-staging", self.calls[0])
        self.assertEqual(self.calls[1], ("dist-tag", "rm", sdk.PACKAGE, "dev-staging"))

    def test_retry_is_idempotent(self):
        self.existing()
        self.metadata["dev"] = {"version": self.package.version}
        sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [])

    def test_retry_can_repair_tag_without_republishing(self):
        self.existing()
        sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [("dist-tag", "add", f"{sdk.PACKAGE}@{self.package.version}", "dev")])

    def test_retry_cleans_up_only_its_staging_tag(self):
        self.existing()
        self.metadata["dev"] = {"version": "0.2.0-dev.43.1"}
        self.metadata["dev-staging"] = {"version": "0.1.0-dev.41.1"}
        sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [])
        self.metadata["dev-staging"]["version"] = self.package.version
        sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [("dist-tag", "rm", sdk.PACKAGE, "dev-staging")])

    def test_existing_version_with_different_bytes_is_rejected(self):
        self.existing(integrity="sha512-different")
        with self.assertRaisesRegex(ValueError, "different package contents"):
            sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [])

    def test_registry_failure_does_not_publish(self):
        with patch.object(sdk, "registry_metadata", side_effect=OSError("registry unavailable")):
            with self.assertRaises(OSError):
                sdk.publish(self.package, self.provenance)
        self.assertEqual(self.calls, [])


class RegistryTest(unittest.TestCase):
    def test_only_404_means_absent(self):
        for code in (404, 403, 500):
            with patch.object(sdk, "urlopen", side_effect=HTTPError("url", code, "failure", {}, None)):
                if code == 404:
                    self.assertIsNone(sdk.registry_metadata("dev"))
                else:
                    with self.assertRaises(HTTPError):
                        sdk.registry_metadata("dev")


if __name__ == "__main__":
    unittest.main()
