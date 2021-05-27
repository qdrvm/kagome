#include "metrics/metrics.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <thread>
#include "metrics/impl/prometheus/registry_impl.hpp"

template <typename T>
prometheus::ClientMetric getMetric(T *metric) {
  return kagome::metrics::PrometheusRegistry::internalMetric(metric)->Collect();
}

class CounterTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Counter *createCounter(const std::string &name) {
    registry_->registerCounterFamily(name);
    return registry_->registerCounterMetric(name);
  }
};

// all tests have same static prometheus::Registry

TEST_F(CounterTest, initialize_with_zero) {
  auto counter = createCounter("counter1");
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 0.0);
}

TEST_F(CounterTest, inc) {
  auto counter = createCounter("counter2");
  counter->inc();
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 1.0);
}

TEST_F(CounterTest, inc_number) {
  auto counter = createCounter("counter3");
  counter->inc(4.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 4.0);
}

TEST_F(CounterTest, inc_multiple) {
  auto counter = createCounter("counter4");
  counter->inc();
  counter->inc();
  counter->inc(5.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 7.0);
}

TEST_F(CounterTest, inc_negative_value) {
  auto counter = createCounter("counter5");
  counter->inc(5.0);
  counter->inc(-5.0);
  EXPECT_DOUBLE_EQ(getMetric(counter).counter.value, 5.0);
}

class GaugeTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Gauge *createGauge(const std::string &name) {
    registry_->registerGaugeFamily(name);
    return registry_->registerGaugeMetric(name);
  }
};

TEST_F(GaugeTest, initialize_with_zero) {
  auto gauge = createGauge("gauge1");
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 0);
}

TEST_F(GaugeTest, inc) {
  auto gauge = createGauge("gauge2");
  gauge->inc();
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

TEST_F(GaugeTest, inc_number) {
  auto gauge = createGauge("gauge3");
  gauge->inc(4);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 4.0);
}

TEST_F(GaugeTest, inc_multiple) {
  auto gauge = createGauge("gauge4");
  gauge->inc();
  gauge->inc();
  gauge->inc(5);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 7.0);
}

TEST_F(GaugeTest, inc_negative_value) {
  auto gauge = createGauge("gauge5");
  gauge->inc(-1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, -1.0);
}

TEST_F(GaugeTest, dec) {
  auto gauge = createGauge("gauge6");
  gauge->set(5.0);
  gauge->dec();
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 4.0);
}

TEST_F(GaugeTest, dec_negative_value) {
  auto gauge = createGauge("gauge7");
  gauge->dec(-1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

TEST_F(GaugeTest, dec_number) {
  auto gauge = createGauge("gauge8");
  gauge->set(5.0);
  gauge->dec(3.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 2.0);
}

TEST_F(GaugeTest, set) {
  auto gauge = createGauge("gauge9");
  gauge->set(3.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 3.0);
}

TEST_F(GaugeTest, set_multiple) {
  auto gauge = createGauge("gauge10");
  gauge->set(3.0);
  gauge->set(8.0);
  gauge->set(1.0);
  EXPECT_DOUBLE_EQ(getMetric(gauge).gauge.value, 1.0);
}

class HistogramTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Histogram *createHistogram(
      const std::string &name, const std::vector<double> &bucket_boundaries) {
    registry_->registerHistogramFamily(name);
    return registry_->registerHistogramMetric(name, bucket_boundaries);
  }
};

TEST_F(HistogramTest, initialize_with_zero) {
  auto histogram = createHistogram("histogram1", {});
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.sample_count, 0U);
  EXPECT_DOUBLE_EQ(h.sample_sum, 0);
}

TEST_F(HistogramTest, sample_count) {
  auto histogram = createHistogram("histogram2", {1});
  histogram->observe(0);
  histogram->observe(200);
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.sample_count, 2U);
}

TEST_F(HistogramTest, sample_sum) {
  auto histogram = createHistogram("histogram3", {1});
  histogram->observe(0);
  histogram->observe(1);
  histogram->observe(101);
  auto h = getMetric(histogram).histogram;
  EXPECT_DOUBLE_EQ(h.sample_sum, 102);
}

TEST_F(HistogramTest, bucket_size) {
  auto histogram = createHistogram("histogram4", {1, 2});
  auto h = getMetric(histogram).histogram;
  EXPECT_EQ(h.bucket.size(), 3U);
}

TEST_F(HistogramTest, bucket_bounds) {
  auto histogram = createHistogram("histogram5", {1, 2});
  auto h = getMetric(histogram).histogram;
  EXPECT_DOUBLE_EQ(h.bucket.at(0).upper_bound, 1);
  EXPECT_DOUBLE_EQ(h.bucket.at(1).upper_bound, 2);
  EXPECT_DOUBLE_EQ(h.bucket.at(2).upper_bound,
                   std::numeric_limits<double>::infinity());
}

TEST_F(HistogramTest, bucket_counts_not_reset_by_collection) {
  auto histogram = createHistogram("histogram6", {1, 2});
  histogram->observe(1.5);
  getMetric(histogram);
  histogram->observe(1.5);
  auto h = getMetric(histogram).histogram;
  ASSERT_EQ(h.bucket.size(), 3U);
  EXPECT_EQ(h.bucket.at(1).cumulative_count, 2U);
}

TEST_F(HistogramTest, cumulative_bucket_count) {
  auto histogram = createHistogram("histogram7", {1, 2});
  histogram->observe(0);
  histogram->observe(0.5);
  histogram->observe(1);
  histogram->observe(1.5);
  histogram->observe(1.5);
  histogram->observe(2);
  histogram->observe(3);
  auto h = getMetric(histogram).histogram;
  ASSERT_EQ(h.bucket.size(), 3U);
  EXPECT_EQ(h.bucket.at(0).cumulative_count, 3U);
  EXPECT_EQ(h.bucket.at(1).cumulative_count, 6U);
  EXPECT_EQ(h.bucket.at(2).cumulative_count, 7U);
}

TEST_F(HistogramTest, sum_can_go_down) {
  auto histogram = createHistogram("histogram8", {1});
  auto histogram1 = getMetric(histogram).histogram;
  histogram->observe(-10);
  auto histogram2 = getMetric(histogram).histogram;
  EXPECT_LT(histogram2.sample_sum, histogram1.sample_sum);
}

class SummaryTest : public ::testing::Test {
  kagome::metrics::RegistryPtr registry_;

 public:
  void SetUp() override {
    registry_ = kagome::metrics::createRegistry();
  }

  kagome::metrics::Summary *createSummary(
      const std::string &name,
      const std::vector<std::pair<double, double>> &quantiles,
      std::chrono::milliseconds max_age = std::chrono::seconds{60},
      int age_buckets = 5) {
    registry_->registerSummaryFamily(name);
    return registry_->registerSummaryMetric(
        name, quantiles, max_age, age_buckets);
  }
};

TEST_F(SummaryTest, initialize_with_zero) {
  auto summary = createSummary("summary1", {});
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.sample_count, 0U);
  EXPECT_DOUBLE_EQ(s.sample_sum, 0);
}

TEST_F(SummaryTest, sample_count) {
  auto summary = createSummary("summary2", {{0.5, 0.05}});
  summary->observe(0);
  summary->observe(200);
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.sample_count, 2U);
}

TEST_F(SummaryTest, sample_sum) {
  auto summary = createSummary("summary3", {{0.5, 0.05}});
  summary->observe(0);
  summary->observe(1);
  summary->observe(101);
  auto s = getMetric(summary).summary;
  EXPECT_DOUBLE_EQ(s.sample_sum, 102);
}

TEST_F(SummaryTest, quantile_size) {
  auto summary = createSummary("summary4", {{0.5, 0.05}, {0.90, 0.01}});
  auto s = getMetric(summary).summary;
  EXPECT_EQ(s.quantile.size(), 2U);
}

TEST_F(SummaryTest, quantile_bounds) {
  auto summary =
      createSummary("summary5", {{0.5, 0.05}, {0.90, 0.01}, {0.99, 0.001}});
  auto s = getMetric(summary).summary;
  ASSERT_EQ(s.quantile.size(), 3U);
  EXPECT_DOUBLE_EQ(s.quantile.at(0).quantile, 0.5);
  EXPECT_DOUBLE_EQ(s.quantile.at(1).quantile, 0.9);
  EXPECT_DOUBLE_EQ(s.quantile.at(2).quantile, 0.99);
}

TEST_F(SummaryTest, quantile_values) {
  static const int SAMPLES = 10000;

  auto summary =
      createSummary("summary6", {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  for (int i = 1; i <= SAMPLES; ++i) summary->observe(i);

  auto s = getMetric(summary).summary;
  ASSERT_EQ(s.quantile.size(), 3U);

  EXPECT_NEAR(s.quantile.at(0).value, 0.5 * SAMPLES, 0.05 * SAMPLES);
  EXPECT_NEAR(s.quantile.at(1).value, 0.9 * SAMPLES, 0.01 * SAMPLES);
  EXPECT_NEAR(s.quantile.at(2).value, 0.99 * SAMPLES, 0.001 * SAMPLES);
}

TEST_F(SummaryTest, max_age) {
  auto summary =
      createSummary("summary7", {{0.99, 0.001}}, std::chrono::milliseconds(80), 2);
  summary->observe(8.0);

  static const auto test_value = [&summary](double ref) {
    auto s = getMetric(summary).summary;
    ASSERT_EQ(s.quantile.size(), 1U);

    if (std::isnan(ref))
      EXPECT_TRUE(std::isnan(s.quantile.at(0).value));
    else
      EXPECT_DOUBLE_EQ(s.quantile.at(0).value, ref);
  };

  test_value(8.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  test_value(8.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  test_value(std::numeric_limits<double>::quiet_NaN());
}
