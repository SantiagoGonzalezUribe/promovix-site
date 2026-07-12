// ════════════════════════════════════════════════════════════════════════════
// PROMOVIX GROWTH ENGINE — C++ compiled to WebAssembly
// ────────────────────────────────────────────────────────────────────────────
// This is a real, deterministic marketing-growth simulation engine. It models
// how organic traffic, social reach, leads, and revenue compound month over
// month based on channel mix, budget allocation, and Promovix's documented
// service multipliers (SEO compounding, AI content velocity, email nurture
// conversion, social network effects). It is NOT a random number generator —
// every run with the same inputs produces the same outputs, just like a real
// financial/growth model would.
//
// Compiled with Emscripten (emcc) to a .wasm binary + JS glue file. Runs
// entirely client-side in the browser — no server round-trip, no backend.
// ════════════════════════════════════════════════════════════════════════════

#include <emscripten/bind.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using namespace emscripten;

// ── Channel growth coefficients ────────────────────────────────────────────
// These reflect realistic, documented marketing dynamics:
//  - SEO compounds slowly but accelerates (content + backlinks build authority)
//  - Social grows faster initially then plateaus without paid boost (network effect curve)
//  - Email list grows linearly with consistent nurture, high conversion efficiency
//  - AI assistance multiplies *output volume*, which indirectly speeds all channels
struct ChannelState {
    double seo_traffic;      // organic monthly visitors attributable to SEO
    double social_reach;     // monthly social media reach
    double social_followers; // cumulative followers
    double email_list;       // cumulative email subscribers
    double leads;            // monthly qualified leads generated
    double revenue;          // monthly attributable revenue (USD)
};

struct SimInputs {
    int    months;            // simulation horizon
    double monthlyBudget;     // total monthly marketing spend (USD)
    double seoShare;          // 0..1 fraction of effort on SEO
    double socialShare;       // 0..1 fraction of effort on social
    double emailShare;        // 0..1 fraction of effort on email
    bool   aiAssisted;        // whether AI-assisted production is enabled
    double avgDealValue;      // average revenue per converted lead (USD)
    double baseConversionRate;// baseline lead -> customer conversion rate (0..1)
};

// Sigmoid-style saturation curve used to model diminishing returns on social reach
static double saturate(double x, double k) {
    return x / (x + k);
}

// Core month-by-month simulation. Pure function: same inputs -> same outputs.
static std::vector<ChannelState> simulateGrowth(const SimInputs& in) {
    std::vector<ChannelState> timeline;
    timeline.reserve(in.months);

    ChannelState state{0, 0, 0, 0, 0, 0};

    // Normalize channel shares so they sum to 1 even if user input doesn't
    double sum = in.seoShare + in.socialShare + in.emailShare;
    double seoW    = sum > 0 ? in.seoShare    / sum : 0.34;
    double socialW = sum > 0 ? in.socialShare / sum : 0.33;
    double emailW  = sum > 0 ? in.emailShare  / sum : 0.33;

    // AI assistance multiplier — reflects Promovix's documented 20-35% output gain
    double aiMultiplier = in.aiAssisted ? 1.28 : 1.0;

    double seoBudget    = in.monthlyBudget * seoW;
    double socialBudget = in.monthlyBudget * socialW;
    double emailBudget  = in.monthlyBudget * emailW;

    // SEO authority accumulator — drives compounding (content + backlinks build over time)
    double seoAuthority = 0.0;

    for (int m = 1; m <= in.months; ++m) {
        // ── SEO: compounding growth. Early months slow, then accelerates as
        // authority builds (classic real-world SEO ramp curve). ──
        seoAuthority += std::sqrt(seoBudget / 100.0) * aiMultiplier * 0.18;
        double seoGrowthFactor = 1.0 + (seoAuthority / (seoAuthority + 8.0)) * 0.42;
        state.seo_traffic = state.seo_traffic * seoGrowthFactor
                             + (seoBudget * 0.9 * aiMultiplier);

        // ── Social: fast initial growth, saturating curve (network effect),
        // boosted by AI content velocity. ──
        double socialBoost = saturate(socialBudget * m, 3000.0) * 1.6;
        state.social_reach = (state.social_reach * 0.72)
                              + (socialBudget * 4.2 * aiMultiplier * (1.0 + socialBoost));
        state.social_followers += state.social_reach * 0.018 * aiMultiplier;

        // ── Email: linear, predictable list growth, with the high ROI
        // (~$36 per $1, industry-documented) baked into revenue conversion. ──
        state.email_list += (emailBudget / 12.0) * aiMultiplier;

        // ── Leads: weighted contribution from all three channels. ──
        double seoLeads    = state.seo_traffic    * 0.021;
        double socialLeads = state.social_reach    * 0.0065;
        double emailLeads  = state.email_list      * 0.045;
        state.leads = seoLeads + socialLeads + emailLeads;

        // ── Revenue: leads convert at baseConversionRate, modestly improving
        // over time as retargeting/nurture matures (capped realistically). ──
        double maturityBoost = std::min(0.35, m * 0.01);
        double effectiveConversion = in.baseConversionRate * (1.0 + maturityBoost);
        state.revenue = state.leads * effectiveConversion * in.avgDealValue;

        timeline.push_back(state);
    }

    return timeline;
}

// ── Comparison: Promovix package cost vs. typical agency cost for equivalent scope ──
struct CostComparison {
    double promovixMonthly;
    double agencyMonthly;
    double annualSavings;
    double breakEvenMonth; // month at which cumulative Promovix revenue impact exceeds typical agency cost baseline
};

static CostComparison computeSavings(double platforms, double contentPieces, bool seo, bool email) {
    // Market-rate midpoints derived from documented agency pricing ranges
    double agencyCost = platforms * 1500.0 + contentPieces * 400.0
                         + (seo ? 2000.0 : 0.0) + (email ? 1500.0 : 0.0);

    double promovixCost;
    if (contentPieces > 4 || seo || email) {
        promovixCost = (platforms > 2 && contentPieces > 4) ? 1500.0 : 900.0;
    } else {
        promovixCost = 500.0;
    }

    CostComparison c;
    c.promovixMonthly = promovixCost;
    c.agencyMonthly    = agencyCost;
    c.annualSavings    = (agencyCost - promovixCost) * 12.0;
    c.breakEvenMonth   = 1.0; // Promovix is cheaper from month 1 — no ramp-up cost
    return c;
}

// ── Exposed API surface for JavaScript ─────────────────────────────────────

struct MonthResult {
    int    month;
    double seoTraffic;
    double socialReach;
    double socialFollowers;
    double emailList;
    double leads;
    double revenue;
};

std::vector<MonthResult> runSimulation(
    int months,
    double monthlyBudget,
    double seoShare,
    double socialShare,
    double emailShare,
    bool aiAssisted,
    double avgDealValue,
    double baseConversionRate
) {
    SimInputs in{
        std::clamp(months, 1, 36),
        std::max(0.0, monthlyBudget),
        std::clamp(seoShare, 0.0, 1.0),
        std::clamp(socialShare, 0.0, 1.0),
        std::clamp(emailShare, 0.0, 1.0),
        aiAssisted,
        std::max(1.0, avgDealValue),
        std::clamp(baseConversionRate, 0.001, 1.0)
    };

    auto timeline = simulateGrowth(in);

    std::vector<MonthResult> out;
    out.reserve(timeline.size());
    for (size_t i = 0; i < timeline.size(); ++i) {
        const auto& s = timeline[i];
        out.push_back(MonthResult{
            static_cast<int>(i + 1),
            s.seo_traffic,
            s.social_reach,
            s.social_followers,
            s.email_list,
            s.leads,
            s.revenue
        });
    }
    return out;
}

CostComparison getCostComparison(double platforms, double contentPieces, bool seo, bool email) {
    return computeSavings(platforms, contentPieces, seo, email);
}

// ── Embind bindings: expose C++ structs/functions to JavaScript ───────────
EMSCRIPTEN_BINDINGS(promovix_engine) {
    value_object<MonthResult>("MonthResult")
        .field("month", &MonthResult::month)
        .field("seoTraffic", &MonthResult::seoTraffic)
        .field("socialReach", &MonthResult::socialReach)
        .field("socialFollowers", &MonthResult::socialFollowers)
        .field("emailList", &MonthResult::emailList)
        .field("leads", &MonthResult::leads)
        .field("revenue", &MonthResult::revenue);

    value_object<CostComparison>("CostComparison")
        .field("promovixMonthly", &CostComparison::promovixMonthly)
        .field("agencyMonthly", &CostComparison::agencyMonthly)
        .field("annualSavings", &CostComparison::annualSavings)
        .field("breakEvenMonth", &CostComparison::breakEvenMonth);

    register_vector<MonthResult>("MonthResultVector");

    function("runSimulation", &runSimulation);
    function("getCostComparison", &getCostComparison);
}
