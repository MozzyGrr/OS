#!/bin/sh


BASE_DIR="$(pwd)"


fail() {
    echo "ERROR: $1" >&2
    exit 1
}


finalize() {
    cd "$BASE_DIR"
    if [ -n "$WORK_DIR" ] && [ -d "$WORK_DIR" ]; then
        rm -rf "$WORK_DIR"
    fi
}


if [ "$#" -ne 1 ]; then
    fail "Скрипт $0 требует ровно один аргумент"
fi


INPUT_PATH="$1"
INPUT_DIR="$(dirname "$INPUT_PATH")"
INPUT_NAME="$(basename "$INPUT_PATH")"

if [ ! -f "$INPUT_PATH" ]; then
    fail "Файл недоступен или не найден: $INPUT_PATH"
fi


if [ "${INPUT_NAME##*.}" != "cpp" ]; then
    fail "Файл с расширением .cpp: $INPUT_PATH"
fi


TARGET_NAME="$(grep 'Comment:' "$INPUT_PATH" | sed 's/.*Comment:[[:space:]]*//')"

if [ -z "$TARGET_NAME" ]; then
    fail "Comment: <имя_файла> не найден"
fi


WORK_DIR="$(mktemp -d XXX)"


trap finalize EXIT HUP INT TERM


cp "$INPUT_PATH" "$WORK_DIR/$INPUT_NAME"


cd "$WORK_DIR"

if g++ -o "$TARGET_NAME" "$INPUT_NAME"; then
    BUILD_OK=true
else
    fail "Ошибка компиляции"
fi


if [ "$BUILD_OK" = true ]; then
    if [ -f "$TARGET_NAME" ]; then
        mv "$TARGET_NAME" "$BASE_DIR/$INPUT_DIR/"
        echo "Готово: $BASE_DIR/$INPUT_DIR/$TARGET_NAME"
    else
        fail "Файл не найден"
    fi
fi

exit 0
