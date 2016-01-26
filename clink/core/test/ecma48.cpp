// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/ecma48_iter.h>
#include <core/base.h>

static ecma48_state g_state;

TEST_CASE("Ecma48 chars") {
    const char* input = "abc123";

    const ecma48_code* code;

    ecma48_iter iter(input, g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_chars);
    REQUIRE(code->str == input);
    REQUIRE(code->length == 6);

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 c0") {
    const ecma48_code* code;

    ecma48_iter iter("\x01 \x10\x1f", g_state);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c0);
    REQUIRE(code->c0 == 0x01);
    REQUIRE(code->length == 1);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_chars);
    REQUIRE(code->length == 1);
    REQUIRE(code->str[0] == ' ');

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c0);
    REQUIRE(code->c0 == 0x10);
    REQUIRE(code->length == 1);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c0);
    REQUIRE(code->c0 == 0x1f);
    REQUIRE(code->length == 1);

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 c1") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b\x40\x1b\x50\x1b\x5f", g_state);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c1);
    REQUIRE(code->c1 == 0x40);
    REQUIRE(code->length == 2);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c1);
    REQUIRE(code->c1 == 0x50);
    REQUIRE(code->length == 2);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c1);
    REQUIRE(code->c1 == 0x5f);
    REQUIRE(code->length == 2);

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 icf") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b\x60\x1b\x7f", g_state);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_icf);
    REQUIRE(code->icf == 0x60);
    REQUIRE(code->length == 2);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_icf);
    REQUIRE(code->icf == 0x7f);
    REQUIRE(code->length == 2);

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 csi") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b[\x40\x1b[\x20\x7e", g_state);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_csi);
    REQUIRE(code->csi->func == 0x40);
    REQUIRE(code->csi->param_count == 0);
    REQUIRE(code->length == 3);

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_csi);
    REQUIRE(code->csi->func == 0x207e);
    REQUIRE(code->csi->param_count == 0);
    REQUIRE(code->length == 4);

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 csi params") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b[1;12;123\x40", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->csi->param_count == 3);
    REQUIRE(code->csi->params[0] == 1);
    REQUIRE(code->csi->params[1] == 12);
    REQUIRE(code->csi->params[2] == 123);
    REQUIRE(code->length == 11);

    new (&iter) ecma48_iter("\x1b[;@", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->csi->param_count == 2);
    REQUIRE(code->csi->params[0] == 0);
    REQUIRE(code->csi->params[1] == 0);
    REQUIRE(code->length == 4);

    new (&iter) ecma48_iter("\x1b[;;;;;;;;;;;;1;2;3;4;5m", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->csi->param_count == 8);
    REQUIRE(code->length == 24);
}

TEST_CASE("Ecma48 csi invalid") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b[1;2\01", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c0);
    REQUIRE(code->c0 == 1);
    REQUIRE(code->str[0] == 1);
    REQUIRE(code->length == 1);
}

TEST_CASE("Ecma48 stream") {
    const ecma48_code* code;

    ecma48_iter iter("\x1b[1;2", g_state);
    REQUIRE(iter.next() == nullptr);

    new (&iter) ecma48_iter("1m", g_state);
    REQUIRE((code = iter.next()) != nullptr);

    REQUIRE(code->type == ecma48_code::type_csi);
    REQUIRE(code->csi->func == 'm');
    REQUIRE(code->csi->param_count == 2);
    REQUIRE(code->csi->params[0] == 1);
    REQUIRE(code->csi->params[1] == 21);
    REQUIRE(code->length == 2);
}

TEST_CASE("Ecma48 split") {
    const ecma48_code* code;

    ecma48_iter iter(" \x1b[1;2x@@@@", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_chars);
    REQUIRE(code->length == 1);
    REQUIRE(code->str[0] == ' ');

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_csi);
    REQUIRE(code->csi->func == 'x');

    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_chars);
    REQUIRE(code->length == 4);
    REQUIRE(code->str[0] == '@');

    REQUIRE(iter.next() == nullptr);
}

TEST_CASE("Ecma48 utf8") {
    const ecma48_code* code;

    ecma48_iter iter("\xc2\x9bz", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_csi);
    REQUIRE(code->csi->func == 'z');
    REQUIRE(code->length == 3);

    new (&iter) ecma48_iter("\xc2\x9c", g_state);
    REQUIRE((code = iter.next()) != nullptr);
    REQUIRE(code->type == ecma48_code::type_c1);
    REQUIRE(code->c1 == 0x5c);
    REQUIRE(code->length == 2);
}