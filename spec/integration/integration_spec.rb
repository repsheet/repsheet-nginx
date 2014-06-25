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
      @redis.set("127.0.0.1:repsheet:blacklist", "true")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(403)
    end

    it "Returns a 403 response if the user is on the blacklist" do
      @redis.sadd("repsheet:users:blacklist", "repsheet")
      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=repsheet"
      end

      expect(http.response_code).to eq(403)
    end

    it "Returns a 200 response if the IP is on the whitelist" do
      @redis.set("127.0.0.1:repsheet:blacklist", "true")
      @redis.set("127.0.0.1:repsheet:whitelist", "true")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
    end

    it "Returns a 200 response if the user is on the whitelist" do
      @redis.sadd("repsheet:users:blacklist", "repsheet")
      @redis.sadd("repsheet:users:whitelist", "repsheet")

      http = Curl.get("http://127.0.0.1:8888") do |http|
        http.headers['Cookie'] = "user=repsheet"
      end

      expect(http.response_code).to eq(200)
    end

    it "Returns a 200 response if the IP is marked on the repsheet" do
      @redis.set("127.0.0.1:repsheet", "true")
      expect(Curl.get("http://127.0.0.1:8888").response_code).to eq(200)
    end

    it "Returns a 200 response if the user is marked on the repsheet" do
      @redis.sadd("repsheet:users", "repsheet")

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
