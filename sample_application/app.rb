require 'sinatra'

get '/app' do
  if request.env['HTTP_X_REPSHEET']
    "Actor is marked. Reason: #{request.env['HTTP_X_REPSHEET']}"
  else
    "Actor is not on the Repsheet"
  end
end
