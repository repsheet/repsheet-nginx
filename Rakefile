require 'rake'
require 'rspec/core/rake_task'
require 'redis'

RSpec::Core::RakeTask.new(:integration) do |t|
  t.pattern = "spec/**/*_spec.rb"
end

namespace :nginx do
  task :start do
    `build/nginx/sbin/nginx`
    sleep 1
  end

  task :stop do
    `build/nginx/sbin/nginx -s stop`
  end

  task :compile do
    sh "script/bootstrap compile"
  end
end

namespace :repsheet do
  task :bootstrap do
    unless Dir.exists?("build") and Dir.exists?("vendor")
      sh "script/bootstrap"
    end
  end
end

desc "Run the integration tests against NGINX"
task :default => ["repsheet:bootstrap", "nginx:start", :integration, "nginx:stop"]
