#!/usr/bin/env python3
"""Executa as 45 medições requeridas e salva os tempos individuais em CSV."""

import csv
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
INPUTS = ("pequena.txt", "media.txt", "grande.txt")
THREAD_CONFIGS = ("2", "4", "8", "max")
REPETITIONS = 3
OUTPUT = ROOT / "resultados_bucket_sort.csv"
TIME_PATTERN = re.compile(r"Tempo de ordenacao: ([0-9]+(?:\.[0-9]+)?) segundos")


def compile_program(command):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(f"Falha ao compilar: {' '.join(command)}")


def execute(command, config, input_name, repetition):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        sys.stderr.write(result.stdout[-2000:])
        raise SystemExit(f"Falha na execução: {' '.join(command)}")

    time_match = TIME_PATTERN.search(result.stdout)
    if not time_match:
        raise SystemExit(f"Tempo não encontrado na saída de {' '.join(command)}")

    actual_threads = "1"
    validation = "N/A"
    if config != "sequencial":
        thread_match = re.search(r"Threads: (\d+)", result.stdout)
        if not thread_match:
            raise SystemExit(f"Número de threads não encontrado: {' '.join(command)}")
        actual_threads = thread_match.group(1)
        if "Verificacao sequencial/paralela: OK" not in result.stdout:
            raise SystemExit(f"Validação sequencial/paralela ausente: {' '.join(command)}")
        validation = "OK"

    return {
        "entrada": input_name,
        "configuracao": config,
        "threads_solicitadas": "1" if config == "sequencial" else config,
        "threads_criadas": actual_threads,
        "repeticao": repetition,
        "tempo_segundos": time_match.group(1),
        "validacao": validation,
    }


def main():
    compile_program(["gcc", "-Wall", "-Wextra", "-O2", "bucket_sort.c", "-o", "bucket_sort"])
    compile_program([
        "gcc", "-Wall", "-Wextra", "-O2", "thread_bucket_sort.c",
        "-o", "thread_bucket_sort", "-pthread",
    ])

    rows = []
    total = len(INPUTS) * (1 + len(THREAD_CONFIGS)) * REPETITIONS
    for input_name in INPUTS:
        input_path = f"entradas/{input_name}"
        for repetition in range(1, REPETITIONS + 1):
            rows.append(execute(
                ["./bucket_sort", input_path], "sequencial", input_name, repetition
            ))
            print(f"[{len(rows)}/{total}] {input_name}, sequencial, repetição {repetition}", file=sys.stderr)

            for config in THREAD_CONFIGS:
                rows.append(execute(
                    ["./thread_bucket_sort", input_path, config],
                    config,
                    input_name,
                    repetition,
                ))
                print(
                    f"[{len(rows)}/{total}] {input_name}, pthreads {config}, repetição {repetition}",
                    file=sys.stderr,
                )

    with OUTPUT.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print(f"Concluído: {len(rows)} medições salvas em {OUTPUT}")


if __name__ == "__main__":
    main()
