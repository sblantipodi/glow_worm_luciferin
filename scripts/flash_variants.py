import argparse
import json
import shutil
import subprocess
from pathlib import Path


MATTER_CHIPS = (
    "ESP32",
    "ESP32-C3",
    "ESP32-C5",
    "ESP32-C6",
    "ESP32-S3",
    "ESP32-S31",
)


def matter_artifact_name(chip, file_name):
    chip_suffix = chip.replace("-", "_")
    return f"GlowWormLuciferinFULL_{chip_suffix}_8MB_{file_name}"


def build_variants(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for chip in MATTER_CHIPS:
        target = f"glowwormluciferinfull_{chip.lower().replace('-', '')}_8mb"
        subprocess.run(["platformio", "run", "-e", target], check=True)
        build_dir = Path(".pio/build") / target
        for file_name in ("firmware.bin", "firmware-factory.bin"):
            shutil.copyfile(build_dir / file_name, destination / matter_artifact_name(chip, file_name))


def write_manifests(directory):
    manifest = json.loads((directory / "manifest_full.json").read_text(encoding="utf-8"))
    matter_builds = manifest.pop("builds_8mb")
    expected_parts = {
        chip: [{"path": matter_artifact_name(chip, "firmware-factory.bin"), "offset": 0}]
        for chip in MATTER_CHIPS
    }
    actual_parts = {build["chipFamily"]: build["parts"] for build in matter_builds}
    if len(matter_builds) != len(expected_parts) or actual_parts != expected_parts:
        raise ValueError("The Full firmware manifest must list one 8 MB factory image for each Matter target")

    variants = {"4mb": manifest, "8mb": {**manifest, "builds": matter_builds}}
    for variant in variants.values():
        for build in variant["builds"]:
            for part in build["parts"]:
                path = directory / part["path"]
                if not path.is_file():
                    raise FileNotFoundError(path)

    for flash_size, variant in variants.items():
        path = directory / f"manifest_full_{flash_size}.json"
        path.write_text(json.dumps(variant, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("build", "manifest"))
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    if args.action == "build":
        build_variants(args.directory)
    else:
        write_manifests(args.directory)
