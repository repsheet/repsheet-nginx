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
    it "Returns a 403 response if the IP is on the blacklist" do
      @redis.set("127.0.0.1:repsheet:ip:blacklist", "Integration Spec")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist" do
      @redis.set("integration:repsheet:users:blacklist", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=integration"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present" do
      @redis.set("integration:repsheet:users:blacklist", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=integration; foo=bar;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present and cookie is at the end" do
      @redis.set("integration:repsheet:users:blacklist", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "foo=bar; user=integration;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist with multiple cookie values present and cookie is in the middle" do
      @redis.set("integration:repsheet:users:blacklist", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "foo=bar; user=integration; baz=quux;"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 403 response if the user is in a blacklisted CIDR block" do
      @redis.set("10.0.0.0/24:repsheet:cidr:blacklist", "Integration Spec")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.15'
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 200 response if the IP is on the whitelist" do
      @redis.set("127.0.0.1:repsheet:ip:blacklist", "Integration Spec")
      @redis.set("127.0.0.1:repsheet:ip:whitelist", "Integration Spec")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
    end

    it "Returns a 200 response if the user is on the whitelist" do
      @redis.set("repsheet:repsheet:users:blacklist", "Integration Spec")
      @redis.set("repsheet:repsheet:users:whitelist", "Integration Spec")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=repsheet"
      end

      expect(http.response_code).to eq(200)
    end

    it "Returns a 200 response if the user is in a blacklisted CIDR block" do
      @redis.set("10.0.0.50:repsheet:ip:blacklist", "Integration Spec")
      @redis.set("10.0.0.0/24:repsheet:cidr:whitelist", "Integration Spec")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['X-Forwarded-For'] = '10.0.0.50'
      end

      expect(http.response_code).to eq(200)
    end

    it "Returns a 200 response if the IP is marked on the repsheet" do
      @redis.set("127.0.0.1:repsheet:ip", "Integration Spec")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
    end

    it "Returns a 200 response if the user is marked on the repsheet" do
      @redis.set("repsheet:repsheet:users", "Integration Spec")

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
  end
end
