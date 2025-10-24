#!/usr/bin/env python3
"""
Converte um modelo TFLite quantizado (int8) em pacote Core ML para uso no app iOS.
"""
import argparse
from pathlib import Path

import coremltools as ct


def convert(tflite_path: Path, labels_path: Path, output_path: Path) -> None:
    model = ct.models.MLModel(tflite_path, convert_to="mlprogram")
    if labels_path.exists():
        labels = [line.strip() for line in labels_path.read_text().splitlines() if line.strip()]
        model.user_defined_metadata["labels"] = ",".join(labels)
    model.save(output_path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tflite", required=True, type=Path)
    parser.add_argument("--labels", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    output_path = args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)

    convert(args.tflite, args.labels, output_path)
    print(f"Modelo convertido salvo em {output_path}")


if __name__ == "__main__":
    main()
