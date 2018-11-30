require 'spec_helper'

describe "Integration Specs" do

  before do
    @redis = Redis.new
    @redis.flushdb
  end

  after  { @redis.flushdb }

  describe "Bootstrap" do
    it "Redis is running" do
      expect(@redis.ping).to eq("PONG")
    end

    it "Webserver is running" do
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
    end
  end

  describe "Actions" do
    context "IPv4" do
      it "Returns a 403 response if the IP is on the blacklist" do
        @redis.set("127.0.0.1:repsheet:ip:blacklisted", "Integration Spec")
        expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(403)
      end

      it "Returns a 200 response if the IP is on the whitelist" do
        @redis.set("127.0.0.1:repsheet:ip:blacklisted", "Integration Spec")
        @redis.set("127.0.0.1:repsheet:ip:whitelisted", "Integration Spec")
        expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
      end

      it "Returns a 200 response if the IP is marked on the repsheet" do
        @redis.set("127.0.0.1:repsheet:ip:marked", "Integration Spec")
        expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
      end
    end

    context "IPv6" do
      it "Returns a 403 response if the IP is on the blacklist" do
        @redis.set("2607:fb90:2c1a:664:0:45:287c:1301:repsheet:ip:blacklisted", "Integration Spec")
        http = Curl.get("http://127.0.0.1:8888") do |http|
          http.headers['X-Forwarded-For'] = "2607:fb90:2c1a:664:0:45:287c:1301"
        end

        expect(http.response_code).to eq(403)
      end

      it "Returns a 200 response if the IP is on the whitelist" do
        @redis.set("2607:fb90:2c1a:664:0:45:287c:1301:repsheet:ip:blacklisted", "Integration Spec")
        @redis.set("2607:fb90:2c1a:664:0:45:287c:1301:repsheet:ip:whitelisted", "Integration Spec")
        http = Curl.get("http://127.0.0.1:8888") do |http|
          http.headers['X-Forwarded-For'] = "2607:fb90:2c1a:664:0:45:287c:1301"
        end

        expect(http.response_code).to eq(200)
      end

      it "Returns a 200 response if the IP is marked on the repsheet" do
        @redis.set("2607:fb90:2c1a:664:0:45:287c:1301:repsheet:ip:marked", "Integration Spec")
        http = Curl.get("http://127.0.0.1:8888") do |http|
          http.headers['X-Forwarded-For'] = "2607:fb90:2c1a:664:0:45:287c:1301"
        end

        expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
      end
    end
  end

  describe "Proxy Filtering" do
    it "Blocks request when invalid addresses are present in X-Forwarded-For" do
      http = Curl.get("http://127.0.0.1:8888?../../") do |http|
        http.headers['X-Forwarded-For'] = '\x5000 8.8.8.8, 12.34.56.78, 98.76.54.32'
      end
      expect(http.response_code).to eq(403)
    end

    it "successfully validates and accepts IPv6 addresses" do
      http = Curl.get("http://127.0.0.1:8888?../../") do |http|
        http.headers['X-Forwarded-For'] = "2607:fb90:2c1a:664:0:45:287c:1301"
      end
      expect(http.response_code).to eq(200)
    end

    it "successfully validates and accepts mixed addresses" do
      http = Curl.get("http://127.0.0.1:8888?../../") do |http|
        http.headers['X-Forwarded-For'] = "2607:fb90:2c1a:664:0:45:287c:1301, 66.249.84.231, 63.80.12.214, 209.170.78.1417"
      end
      expect(http.response_code).to eq(200)
    end
  end

  describe "Alternate XFF header" do
    it "Properly extracts & blocks the alternate header when it's blacklisted" do
      @redis.set("1.1.1.1:repsheet:ip:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888/real") do |http|
        http.headers['True-Client-IP'] = '1.1.1.1'
      end
      expect(http.response_code).to eq(403)
    end

    it "Doesn't block an unblocked IP with true-client-ip set" do
      @redis.set("1.1.1.1:repsheet:ip:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888/real") do |http|
        http.headers['True-Client-IP'] = '2.2.2.2'
      end
      expect(http.response_code).to eq(200)
    end

    it "Falls back to X-Forwarded-For when the alternate header doesn't contain a valid IP and the fallback option is set" do
      @redis.set("1.1.1.1:repsheet:ip:whitelisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888/real") do |http|
        http.headers['True-Client-IP'] = '-'
        http.headers['X-Forwarded-For'] = '1.1.1.1, 2.2.2.2, 3.3.3.3'
      end
      expect(http.response_code).to eq(200)
    end

    it "Does not fall back to X-Forwarded-For when the fallback option is set to off" do
      @redis.set("1.1.1.1:repsheet:ip:whitelisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888/nofallback") do |http|
        http.headers['True-Client-IP'] = '-'
        http.headers['X-Forwarded-For'] = '1.1.1.1, 2.2.2.2, 3.3.3.3'
      end
      expect(http.response_code).to eq(403)
    end
  end
end
