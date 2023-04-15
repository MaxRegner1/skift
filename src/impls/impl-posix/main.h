#pragma once

#include <karm-main/base.h>
#include <karm-media/bundle.h>
#include <karm-sys/chan.h>

int main(int argc, char const **argv) {
    Ctx ctx;
    ctx.add<ArgsHook>(argc, argv);
    ctx.add<Media::BundleHook>(makeStrong<Media::DummyBundle>());
    Res<> code = entryPoint(ctx);

    if (not code) {
        ::Karm::Error error = code.none();
        (void)::Karm::Fmt::format(::Karm::Sys::err(), "{}: {}\n", argv[0], error.msg());
        return 1;
    }

    return 0;
}
