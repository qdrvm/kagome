/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_IMPL_KUSAMA_HARD_FORKS_HPP
#define KAGOME_CONSENSUS_GRANDPA_IMPL_KUSAMA_HARD_FORKS_HPP

#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/impl/schedule_node.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "primitives/ss58_codec.hpp"

namespace kagome::consensus::grandpa {
  inline const primitives::AuthorityList &kusamaHardForksAuthorities() {
    static auto authorities = [] {
      std::vector<std::string_view> ids_str{
          "CwjLJ1zPWK5Ao9WChAFp7rWGEgN3AyXXjTRPrqgm5WwBpoS",
          "Dp8FHpZTzvoKXztkfrUAkF6xNf6sjVU5ZLZ29NEGUazouou",
          "DtK7YfkhNWU6wEPF1dShsFdhtosVAuJPLkoGhKhG1r5LjKq",
          "FLnHYBuoyThzqJ45tdb8P6yMLdocM7ir27Pg1AnpYoygm1K",
          "FWEfJ5UMghr52UopgYjawAg6hQg3ztbQek75pfeRtLVi8pB",
          "ECoLHAu7HKWGTB9od82HAtequYj6hvNHigkGSB9g3ApxAwB",
          "GL1Tg3Uppo8GYL9NjKj4dWKcS6tW98REop9G5hpu7HgFwTa",
          "ExnjU5LZMktrgtQBE3An6FsQfvaKG1ukxPqwhJydgdgarmY",
          "CagLpgCBu5qJqYF2tpFX6BnU4yHvMGSjc7r3Ed1jY3tMbQt",
          "DsrtmMsD4ijh3n4uodxPoiW9NZ7v7no5wVvPVj8fL1dfrWB",
          "HQB4EctrVR68ozZDyBiRJzLRAEGh1YKgCkAsFjJcegL9RQA",
          "H2YTYbXTFkDY1cGnv164ecnDT3hsD2bQXtyiDbcQuXcQZUV",
          "H5WL8jXmbkCoEcLfvqJkbLUeGrDFsJiMXkhhRWn3joct1tE",
          "DpB37GDrJDYcmg2df2eqsrPKMay1u8hyZ6sQi2FuUiUeNLu",
          "FR8yjKRA9MTjvFGK8kfzrdC23Fr6xd7rfBvZXSjAsmuxURE",
          "DxHPty3B9fpj3duu6Gc6gCSCAvsydJHJEY5G3oVYT8S5BYJ",
          "DbVKC8ZJjevrhqSnZyJMMvmPL7oPPL4ed1roxawYnHVgyin",
          "DVJV81kab2J6oTyRJ9T3NCwW2DSrysbWCssvMcE6cwZHnAd",
          "Fg4rDAyzoVzf39Zo8JFPo4W314ntNWNwm3shr4xKe8M1fJg",
          "GUaNcnAruMVxHGTs7gGpSUpigRJboQYQBBQyPohkFcP6NMH",
          "J4BMGF4W9yWiJz4pkhQW73X6QMGpKUzmPppVnqzBCqw5dQq",
          "E1cR61L1tdDEop4WdWVqcq1H1x6VqsDpSHvFyUeC41uruVJ",
          "GoWLzBsj1f23YtdDpyntnvN1LwXKhF5TEeZvBeTVxofgWGR",
          "CwHwmbogSwtRbrkajVBNubPvWmHBGU4bhMido54M9CjuKZD",
          "FLT63y9oVXJnyiWMAL4RvWxsQx21Vymw9961Z7NRFmSG7rw",
          "FoQ2y6JuHuHTG4rHFL3f2hCxfJMvtrq8wwPWdv8tsdkcyA8",
          "D7QQKqqs8ocGorRA12h4QoBSHDia1DkHeXT4eMfjWQ483QH",
          "J6z7FP35F9DiiU985bhkDTS3WxyeTBeoo9MtLdLoD3GiWPj",
          "EjapydCK25AagodRbDECavHAy8yQY1tmeRhwUXhVWx4cFPv",
          "H8admATcRkGCrF1dTDDBCjQDsYjMkuPaN9YwR2mSCj4DWMQ",
          "FtHMRU1fxsoswJjBvyCGvECepC7gP2X77QbNpyikYSqqR6k",
          "DzY5gwr45GVRUFzRMmeg8iffpqYF47nm3XbJhmjG97FijaE",
          "D3HKWAihSUmg8HrfeFrftSwNK7no261yA9RNr3LUUdsuzuJ",
          "D82DwwGJGTcSvtB3SmNrZejnSertbPzpkYvDUp3ibScL3ne",
          "FTPxLXLQvMDQYFA6VqNLGwWPKhemMYP791XVj8TmDpFuV3b",
          "FzGfKmS7N8Z1tvCBU5JH1eBXZQ9pCtRNoMUnNVv38wZNq72",
          "GDfm1MyLAQ7Rh8YPtF6FtMweV4hz91zzeDy2sSABNNqAbmg",
          "DiVQbq7sozeKp7PXPM1HLFc2m7ih8oepKLRK99oBY3QZak1",
          "HErWh7D2RzrjWWB2fTJfcAejD9MJpadeWWZM2Wnk7LiNWfG",
          "Es4DbDauYZYyRJbr6VxrhdcM1iufP9GtdBYf3YtSEvdwNyb",
          "EBgXT6FaVo4WsN2LmfnB2jnpDFf4zay3E492RGSn6v1tY99",
          "Dr9Zg4fxZurexParztL9SezFeHsPwdP8uGgULeRMbk8DDHJ",
          "JEnSTZJpLh91cSryptj57RtFxq9xXqf4U5wBH3qoP91ZZhN",
          "DqtRkrmtPANa8wrYR7Ce2LxJxk2iNFtiCxv1cXbx54uqdTN",
          "GaxmF53xbuTFKopVEseWiaCTa8fC6f99n4YfW8MGPSPYX3s",
          "EiCesgkAaighBKMpwFSAUdvwE4mRjBjNmmd5fP6d4FG8DAx",
          "HVbwWGUx7kCgUGap1Mfcs37g6JAZ5qsfsM7TsDRcSqvfxmd",
          "G45bc8Ajrd6YSXav77gQwjjGoAsR2qiGd1aLzkMy7o1RLwd",
          "Cqix2rD93Mdf7ytg8tBavAig2TvhXPgPZ2mejQvkq7qgRPq",
          "GpodE2S5dPeVjzHB4Drm8R9rEwcQPtwAspXqCVz1ooFWf5K",
          "CwfmfRmzPKLj3ntSCejuVwYmQ1F9iZWY4meQrAVoJ2G8Kce",
          "Fhp5NPvutRCJ4Gx3G8vCYGaveGcU3KgTwfrn5Zr8sLSgwVx",
          "GeYRRPkyi23wSF3cJGjq82117fKJZUbWsAGimUnzb5RPbB1",
          "DzCJ4y5oT611dfKQwbBDVbtCfENTdMCjb4KGMU3Mq6nyUMu",
      };
      primitives::AuthorityList authorities;
      crypto::HasherImpl hasher;
      for (auto &id_str : ids_str) {
        authorities.emplace_back(primitives::Authority{
            {primitives::decodeSs58(id_str, hasher).value()},
            1,
        });
      }
      return authorities;
    }();
    return authorities;
  }

  inline bool isKusamaHardFork(const primitives::BlockHash &genesis,
                               const primitives::BlockInfo &block) {
    auto h = [](std::string_view s) {
      return primitives::BlockHash::fromHex(s).value();
    };
    static auto kusama_genesis =
        h("b0a8d493285c2df73290dfb7e61f870f17b41801197a149ca93654499ea3dafe");
    if (genesis != kusama_genesis) {
      return false;
    }
    if (block.number < 1492283 || block.number > 1498598) {
      return false;
    }
    static std::unordered_set<primitives::BlockHash> blocks{
        h("01e94e1e7e9cf07b3b0bf4e1717fce7448e5563901c2ef2e3b8e9ecaeba088b1"),
        h("ddc4323c5e8966844dfaa87e0c2f74ef6b43115f17bf8e4ff38845a62d02b9a9"),
        h("38ba115b296663e424e32d7b1655cd795719cef4fd7d579271a6d01086cf1628"),
        h("f3172b6b8497c10fc772f5dada4eeb1f4c4919c97de9de2e1a439444d5a057ff"),
        h("b26526aea299e9d24af29fdacd5cf4751a663d24894e3d0a37833aa14c58424a"),
        h("3980d024327d53b8d01ef0d198a052cd058dd579508d8ed6283fe3614e0a3694"),
        h("31f22997a786c25ee677786373368cae6fd501fd1bc4b212b8e267235c88179d"),
        h("1c65eb250cf54b466c64f1a4003d1415a7ee275e49615450c0e0525179857eef"),
        h("9e44116467cc9d7e224e36487bf2cf571698cae16b25f54a7430f1278331fdd8"),
    };
    return blocks.count(block.hash) != 0;
  }

  inline void fixKusamaHardFork(const primitives::BlockHash &genesis,
                                ScheduleNode &node) {
    if (not isKusamaHardFork(genesis, node.block)) {
      return;
    }
    auto action = boost::get<ScheduleNode::ScheduledChange>(&node.action);
    if (not action) {
      throw std::logic_error{"fixKusamaHardfork expected ScheduledChange"};
    }
    action->new_authorities = std::make_shared<primitives::AuthoritySet>(
        action->new_authorities->id, kusamaHardForksAuthorities());
  }
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_IMPL_KUSAMA_HARD_FORKS_HPP
