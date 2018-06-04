# Second example: reading time-stamped data

by Mauri Favaron

## Introduction

### Time stamps, and their importance in data logging

_Time stamp_ is a date and time value, attached as a label to a data item and positioning it with respect to time.

Not all data need to be associated with a time stamp; for example, weight measurements collected during a population survey are usually not time stamped, the only data collected with time being in case the date when measurements were taken, maybe for billing purposes, or under the need to discriminate among different treatments.

There _do_ exist cases, however, in which the precise knowledge of the date and time associated to a data item is of paramount importance. This is the case, for example, of the time series of individual temperature measurements: time here is very important, as it allows sorting measurements in a "before" and "after". It is this time to allow deciding, eventually, to which hourly or daily average does a specific individual data item contribute.

So time stamping is important, in many cases. Another interesting question is, what can you do after you got a time stamp. Here are a few uses:

* If you want to compute wide-span time statistics (for example hourly averages) starting from fine-grained "raw" data, the latter must be all time stamped so that you can easily understand what averaging period do they actually belong - and then the "arithmetical box" you may fit them in.
* Or, maybe, you have two instruments each providing its own observation set from a same phenomenon, or two among which some relation exists, or is to be investigated. In these cases you should make sure the two observation series refer to a unique time base. Or, maybe, to two distinct time bases, but whose reciprocal shift is known. Here too you need time stamps in both series, and a good sorting algorithm.
* Or, perhaps, you may want to analyse a long measurements series looking for seasonal or daily patterns (think a biorhythm, like the human "monthly" period, the sleep-awake alternation, the stomatal activity cycle of a plant, or the yearly change of the nocturnal average of heart beat rate, or the evolution of temperature along days on a month). If the cycle span is regular enough (which rules the human "monthly" period quite out) you may want to use a "typical period" in which you consider time-based classes basically as if they are categorical data, whose computation once again demands knowledge of time stamps.

These are just few examples, and you may yourself devise many more. The task is not difficult, and this lack of difficulty has very much to do with our shared passion for clocks, mechanical and virtual. We love to control time (or illude to), and so the same measurement "with time stamp" gains a big value compared to the same without.

### Types of time stamps

If the concept of time stamp itself looks trivial on the first instance (it is not, as we'll see soon), the objects it's attributed shows quite a considerable span.

A time stamp may, for example, be attributed to an event so concentrated in time to be safely considered an instant.

Or, it may be attributed to an interval as a whole.

The first case is conceptually simple (and technically tricky): all a data logger should do is to find out a time label from its RTC whenever it is needed, the more accurately possible.

The second, connected typically with relatively long averaging times, is conversely technically simple, but inherently ambiguous. The troubles originate from the fact an interval without gaps inside is delimited by _two_ time stamps, let's say them $$t_{-}$$ and $$t_{+}$$ respectively, with $$t_{-} < t_{+}$$, with belonging times $$t$$ satisfying the relation $$t_{-} \le t < t_{+}$$.

In practice, however, a _single_ time stamp $$t_{0}$$ is used, and so the problem arises about when it does exactly occur in the interval. Is maybe $$t_{0} = t_{-}$$? Or perhaps $$t_{0} = t_{+}$$? $$t_{0} = \frac{t_{-}+t_{+}}{2}$$? Schools of thoughts abound, and none of the possibilities (but maybe stamping both the beginning and the end of time interval) provides a definitive answer.

### The inherent difficulty in time-stamping instantaneous events

Assigning a data item an instantaneous time stamp may prove less trivial than one might expect on a first instance.

This is, because of the inherent (and mostly hidden) complexity of the data acquisition process. We'll see it, with reference to the simple case of labeling with a time stamp a threshold exceedance by temperature in a ideal climate chamber.

Here are the phases:

![Data acquisition chain](/Users/maurifavaron/Documents/Research/Hypatia/acq_process.png)

Let's assume the "natural event" is a temperature step change, occurring (say) when an operator pushes a button. As the climate chamber is "ideal", we can assume the step change occurs all of a sudden, immediately as the button is pushed. But as the operator has her/his own times and priority, as far as we're concerned the step change occurs "anywhen in time".

The first step of the data acquisition phase is "detecting" the change. This happens due to a sensor's _primary transducer_ like a resistive thermometer whose resistance exceeds a threshold corresponding to the temperature we want to see crossed.

The sensor, however, behaves as a _first-order instrument_, that is, its response may be described by a first-order linear differential equation, which implies a finite _response time_. This in practice means the primary transducer's resistance will cross the threshold value somewhat _after_ the real event. Because of this, the time stamp assigned to the threshold crossing event will be something like
$$
T_{s} = T_{0} + \varepsilon_{1}
$$
where $$T_{0}$$ is the time instant at which the operator has pressed the pushbutton, and $$\varepsilon_{1}$$ is the response time of the primary transducer.

The event timing, at this time, has not yet become a real time stamp, but quite rather the possibility of it. For the resistance threshold crossing event to become a time stamp, it should be converted to a voltage and encoded as a number using an Analog-to-Digital Converter (ADC), and subsequently compared to the threshold and acknowledged larger by some computer program on board of the datalogger, and finally archived.

All these times contribute to the time stamp, whose expression may be then written as
$$
T_{s} = T_{0} + \varepsilon_{1} + \varepsilon_{2}
$$
So we see the attributed time stamp is an overestimate of the real instant in which the event occurred. The question is, whether the overestimation amount is to be treated as systematic, or as a random variable instead.

From a theoretical standpoint the answer looks simple: all phases involved are deterministic, and so their combination is too: provided enough details are available, the sum $$\varepsilon_{1} + \varepsilon_{2}$$ could be computed exactly.

The real world is a bit nastier than so, however, and "collecting enough details" may prove extremely difficult even in so a simple case. For example, the sampling of electrical signals by the data logger analog to digital circuit occurs at fixed instants which bear no relationship with the original threshold-exceeding event.

Because of this, it makes all sense to treat the sum $$\varepsilon_{1} + \varepsilon_{2}$$ as a random number, whose distribution and parameters may be in principle estimated if not measured directly.

Estimating the "latency time" of a time stamp is quite an art, and some advanced data acquisition system even have dedicated input channels to accomplish this requirement. Our very simple data reader is not that advanced, however, and so we must stay aware of the problem, and keen to anticipate its consequences.

_En passant_, I mention the fact that one consequence is that it is not in general possible to use an uniform time stamp for data collected using different loggers. This is not an important fact for our application, but in general is an issue, and countermeasures (e.g. by adopting a unique time base via an NTP server, or a GPS) tend to be non-trivial (read: expensive).


































































































































































































































































































































































































































































































































































































































































































































































