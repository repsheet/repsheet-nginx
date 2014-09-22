# Repsheet NGINX Module


> **noun**  *Slang.*
> A record kept by web site authorities of a machine's wrongdoings.
>
> **noun**  *Slang.*
> A necessary tool in your web defense arsenal.
>
> **Origin:**
> [2013]

Repsheet is a collection of tools to help improve awareness of robots and bad actors visiting your web applications. It is primarily composed of a core library, web server module, a backend automation tool, and a web based visualization application.

## What problem does it solve?

Repsheet attempts to solve the problem of automated defenses against robots and noisy attackers. Essentially, it is a local reputation engine. As actors visit your application, the Repsheet module looks them up in the cache. It is capable of looking up actors by IP address or by cookie. If Repsheet finds the actor in the cache, it will act in one of the following ways:

* Whitelist - allows the actor to proceed
* Blacklist - returns a `403` response
* Marked - notifies the downstream application of suspect behavior

The following diagram shows a complete Repsheet installation system diagram. The backend and external reputation feeds are optional components.


![Repsheet.png](Repsheet.png)
