#include <array>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/runtime.h>
#include <rex/system/processor.h>

extern "C" PPC_FUNC(sub_827EEDB8);
extern "C" PPC_FUNC(sub_827FE360);

namespace liberty_rex {
namespace {

void ctor_call_827FE360(PPCContext& ctx, uint8_t* base, const std::int64_t r3,
                        const std::int64_t r4, const std::int64_t r5,
                        const std::int64_t r6) {
    ctx.r3.s64 = r3;
    ctx.r4.s64 = r4;
    ctx.r5.s64 = r5;
    ctx.r6.s64 = r6;
    sub_827FE360(ctx, base);
}

void ctor_call_827EEDB8(PPCContext& ctx, uint8_t* base, const std::int64_t r3,
                        const std::int64_t r4, const std::int64_t r5,
                        const std::int64_t r6) {
    ctx.r3.s64 = r3;
    ctx.r4.s64 = r4;
    ctx.r5.s64 = r5;
    ctx.r6.s64 = r6;
    sub_827EEDB8(ctx, base);
}

PPC_FUNC(sub_829F6F60) {
    ctor_call_827FE360(ctx, base, -2096168960 + 6696, -2113470464 + -26568,
                       -2103050240 + -20080, 8357);
}

PPC_FUNC(sub_829F6F80) {
    ctor_call_827FE360(ctx, base, -2096168960 + 6660, -2113470464 + -26536,
                       -2103050240 + -11720, 126);
}

PPC_FUNC(sub_829F6FA0) {
    ctor_call_827FE360(ctx, base, -2096168960 + 6628, -2113470464 + -26512,
                       -2103050240 + -11592, 126);
}

PPC_FUNC(sub_829F6FC0) {
    ctor_call_827EEDB8(ctx, base, -2096168960 + 6712, 0, -2113470464 + -26484, 0);
}

PPC_FUNC(sub_829F7020) {
    ctor_call_827EEDB8(ctx, base, -2096168960 + 6836, 0, -2113470464 + -25064, 0);
}

PPC_FUNC(sub_829F70E0) {
    ctor_call_827FE360(ctx, base, -2096168960 + 9252, -2113470464 + -19180,
                       -2103050240 + 616, 21798);
}

PPC_FUNC(sub_829F7100) {
    ctx.r10.s64 = -2096037888;
    ctx.r11.s64 = -2103050240 + 23332;
    ctx.r9.u64 = PPC_LOAD_U32(ctx.r10.u32 + -7772);
    PPC_STORE_U32(ctx.r10.u32 + -7772, ctx.r11.u32);
    PPC_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
}

PPC_FUNC(sub_829F71E8) {
    ctx.r10.s64 = -2096037888;
    ctx.r11.s64 = -2102984704 + 2840;
    ctx.r9.u64 = PPC_LOAD_U32(ctx.r10.u32 + -7772);
    PPC_STORE_U32(ctx.r10.u32 + -7772, ctx.r11.u32);
    PPC_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
}

PPC_FUNC(sub_829F9DC8) {
    ctor_call_827EEDB8(ctx, base, -2095972352 + 20132, 0, -2113404928 + 12948, 0);
}

struct ForcedCtorTarget {
    std::uint32_t guest;
    PPCFunc* host;
};

constexpr std::array<ForcedCtorTarget, 9> kForcedCtorTargets{{
    {0x829F6F60, sub_829F6F60},
    {0x829F6F80, sub_829F6F80},
    {0x829F6FA0, sub_829F6FA0},
    {0x829F6FC0, sub_829F6FC0},
    {0x829F7020, sub_829F7020},
    {0x829F70E0, sub_829F70E0},
    {0x829F7100, sub_829F7100},
    {0x829F71E8, sub_829F71E8},
    {0x829F9DC8, sub_829F9DC8},
}};

}  // namespace

bool InstallForcedCtorTargets(rex::Runtime& runtime) {
    auto* processor = runtime.processor();
    if (processor == nullptr) {
        REXLOG_ERROR("Forced ctor target audit: missing ReXGlue processor");
        return false;
    }

    for (const auto& target : kForcedCtorTargets) {
        processor->SetFunction(target.guest, target.host);
    }

    REXLOG_INFO("Forced ctor target audit: registered {} sidecar dynamic constructor targets",
                kForcedCtorTargets.size());
    return true;
}

}  // namespace liberty_rex
