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

    it "Returns a 403 response if the user is on the blacklist" do
      @redis.set("integration:repsheet:user:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=integration"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present" do
      @redis.set("integration:repsheet:user:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=integration; foo=bar;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present and cookie is at the end" do
      @redis.set("integration:repsheet:user:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "foo=bar; user=integration;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present and cookie is in the middle" do
      @redis.set("integration:repsheet:user:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "foo=bar; user=integration; baz=quux;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 200 then a 403 response after blacklisting a CIDR block - cache" do
      @redis.set("10.0.0.0/24:repsheet:cidr:blacklisted", "Integration Spec")
      @redis.sadd("repsheet:cidr:blacklisted", "10.0.0.0/24")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.15'
      end

      expect(http.response_code).to eq(200)
      sleep 5

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.15'
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 then a 200 response after whitelisting a blacklisted CIDR block - cache" do
      @redis.set("10.0.0.50:repsheet:ip:blacklisted", "Integration Spec")
      @redis.set("10.0.0.0/24:repsheet:cidr:whitelisted", "Integration Spec")
      @redis.sadd("repsheet:cidr:whitelisted", "10.0.0.0/24")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.50'
      end

      expect(http.response_code).to eq(403)

      sleep 5

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.50'
      end

      expect(http.response_code).to eq(200)
    end

    it "Returns a 200 response if the user is on the whitelist" do
      @redis.set("repsheet:repsheet:user:blacklisted", "Integration Spec")
      @redis.set("repsheet:repsheet:user:whitelisted", "Integration Spec")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=repsheet"
      end

      expect(http.response_code).to eq(200)
    end

    it "Returns a 200 response if the user is marked on the repsheet" do
      @redis.set("repsheet:repsheet:user:marked", "Integration Spec")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=repsheet"
      end

      expect(http.response_code).to eq(200)
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

  describe "Auto Blacklist" do
    it "Blacklists a request when enabled" do
      expect(Curl.get("http://127.0.0.1:8888/blacklist").response_code).to eq(403)
      expect(@redis.get("127.0.0.1:repsheet:ip:blacklisted")).to eq("bad robot")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(403)
    end
  end

  describe "Auto Mark" do
    it "Blacklists a request when enabled" do
      expect(Curl.get("http://127.0.0.1:8888/mark").response_code).to eq(404)
      expect(@redis.get("127.0.0.1:repsheet:ip:marked")).to eq("bad robot")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
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

    it "doesn't block an unblocked IP with true-client-ip set" do
      @redis.set("1.1.1.1:repsheet:ip:blacklisted", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888/real") do |http|
        http.headers['True-Client-IP'] = '2.2.2.2'
      end
      expect(http.response_code).to eq(200)
    end

  end
end
