"""Guard the workflow boundaries required by npm provenance and Web deployment."""
from pathlib import Path
import re
import unittest


WORKFLOWS = Path(__file__).resolve().parents[2] / "workflows"


def workflow_job(filename, job_id):
    # These workflows use two-space job keys. Inspect only the relevant job,
    # so a step-level continue-on-error cannot satisfy the job-level contract.
    source = (WORKFLOWS / filename).read_text(encoding="utf-8")
    pattern = rf"^  {re.escape(job_id)}:[ \t]*\n(.*?)(?=^  [\w-]+:[ \t]*\n|\Z)"
    match = re.search(pattern, source, re.MULTILINE | re.DOTALL)
    if not match:
        raise ValueError(f"Missing job {job_id!r} in {filename}")
    return match[1]


class ProvenanceWorkflowTest(unittest.TestCase):
    def test_source_signing_waits_for_build_and_cannot_fail_web_deployment(self):
        job = workflow_job("build-emscripten.yml", "npm-provenance")
        header = job.split("    steps:", 1)[0]
        self.assertRegex(header, r"(?m)^    needs: build$")
        self.assertRegex(header, r"(?m)^    continue-on-error: true$")
        for guard in ("github.repository == 'DiligentGraphics/DiligentEngine'",
                      "github.event_name == 'push'", "github.ref == 'refs/heads/master'"):
            self.assertIn(guard, header)
        # Without a status override, GitHub also requires needs.build to succeed.
        self.assertNotRegex(header, r"\b(?:always|failure|cancelled)\s*\(")

    def test_source_signing_uses_canonical_provenance_and_its_own_attempt(self):
        job = workflow_job("build-emscripten.yml", "npm-provenance")
        self.assertIn("uses: actions/attest@", job)
        self.assertNotRegex(job, r"(?m)^\s+predicate-(?:type|path):")
        self.assertNotIn("sigstore attest", job)
        self.assertIn('--run "$GITHUB_RUN_NUMBER" --attempt "$GITHUB_RUN_ATTEMPT"', job)
        self.assertIn('--commit "$GITHUB_SHA"', job)
        self.assertIn("name: RadientNpmPackage-${{github.run_number}}-${{github.run_attempt}}-provenance", job)
        self.assertIn("path: ${{steps.npm_provenance.outputs.bundle-path}}", job)

    def test_publisher_uses_source_artifacts_and_does_not_resign_them(self):
        job = workflow_job("publish-npm.yml", "publish-dev")
        self.assertIn("run-id: ${{github.event.workflow_run.id}}", job)
        self.assertIn("pattern: RadientNpmPackage-${{github.event.workflow_run.run_number}}-*", job)
        self.assertIn("merge-multiple: false", job)
        for variable, field in (("SOURCE_RUN_ID", "id"), ("SOURCE_RUN_NUMBER", "run_number"),
                                ("SOURCE_RUN_ATTEMPT", "run_attempt"), ("SOURCE_COMMIT", "head_sha")):
            self.assertIn(f"{variable}: ${{{{github.event.workflow_run.{field}}}}}", job)
        self.assertIn('--run-id "$SOURCE_RUN_ID" --run "$SOURCE_RUN_NUMBER" --attempt "$SOURCE_RUN_ATTEMPT"', job)
        self.assertIn('--commit "$SOURCE_COMMIT"', job)
        self.assertNotIn("uses: actions/attest@", job)
        self.assertNotIn("sigstore attest", job)


if __name__ == "__main__":
    unittest.main()
