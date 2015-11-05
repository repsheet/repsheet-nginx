require 'sinatra'

get '/app' do
  if request.env['HTTP_X_REPSHEET'] == "true"
    "Actor is on the Repsheet"
  else
    "Actor is not on the Repsheet"
  end
end
