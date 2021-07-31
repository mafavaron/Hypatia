# The Hypatia Project

by Patrizia Favaron (development engineer at Servizi Territorio srl)

## What is it?

The Hypatia project is aimed at collecting together some ideas, of practical nature, for making the data-logging process more informative, and more useful.

## Why is it important?

In fact: a huge part of scientific research is based on data collection made automatically by measurement chains whose only visible outcome are the numbers they record - and us, as researcher, sooner or later publish.

Not having the possibility to understand what happens behind the scenes during the data logging process is not necessarily acceptable. As researchers we are responsible to guarantee the correctness of the figures we place in scientific papers, but, how this can happen if the data collection itself is not transparent?

The hard truth is, we trust our data acquisition systems. And sure many of them are robust and high quality enough for us to rely on their function entirely. But this assumption is based on, well, reputation after all. Grounding something on reputation only is quite dangerous and, since the times of Galileo Galilei, unacceptable in scientific research: with data loggers we had to rely on fame and good reputation of measurement chains, but just because we had no alternatives.

The Hypatia Project has the (modest) ambition to show this reliance to not be necessary.

## Why the name, Hypatia?

Hypatia of Alexandria (? - 415 AD) was a renown astronomer, mathematician and philosopher, in her times.

Most of her works have been forgotten since her murder by the hands of religious zealots who did not tolerate a woman did "teach" things to the people, something they imagined to be reserved to priests. But sure she wrote, and her works were highly influential, in her time and space.

The reason of the Hypatia Project name, however, is not simply an homage to one of the most important intellectuals of antiquity (although such a fascinating and off-standing figure fully devises it). More modestly, it comes from one of her most beautiful accomplishments: the public teaching of philosophy she and her disciples made in the streets of Alexandria of Egypt. Public lessons, who no one was obliged to attend: but if they did love knowledge, then they had an opportunity to do.

In our special case, "knowledge" declines in information about data collection and processing process. Information telling to anyone willing to know what exactly happened, what the measurement variability (i.e. noise) was during the data collection, and more.

With this knowledge in their hand, willing people have more reliable instruments to perform data validation, quality assurance, instrument failure identification, and more - to the limits only imagination can place.

## What, exactly, will I find in this space?

In this repository you will find documents and data logger coding examples.

But more, and this is the real purpose of this space, some engineering ideas "in the deep", starting from an apparently simple problem, and delving into the "usefulness" of this all.

Maybe, in future, a specification for an _ideal_ scientific data logger and measurement chain.

And sure, to date, tutorials and examples.

To have an idea of what this project is really about, you may read the very first section of the tutorial "1_SimpleDataReader.pdf" - you find the whole text in the "doc" directory, but to make things simpler to you, and more open, I've also included in this README: just go to last section.

## Following the examples, and improving them

Anyone willing to try can. Most examples in this repository work on the Arduino MKRZERO, a SAMD21-based tiny logger designed and produced by AdaFruit Industries, and readily available. The reason I've chosen it are it's inexpensive, fully open-source hardware (all schematics and tech docs can be downloaded from the AdaFruit site), tiny enough to remain uninvasive, and with a standard LiPo battery-in connector. And, last but not least, accompanied by a wealth of lovely tutorials and examples, the way LadyAda and her colleagues do.

# Excerpt from First example: a simple temperature and relative humidity reader, and some of its problems ("doc/1_SimpleDataReader.pdf")

by: Mauri Favaron (development engineer at Servizi Territorio srl)

## Foreword: why this example?

This document starts with an Arduino sketch. This, just to stay practical, allowing you to grasp, see, touch, and measure yourself.

The intention however is not to "teach" you some micro controller coding, or few elements of digital design - I assume you know this all on yourself. The intention, quite rather, is using techniques and concept familiar to you as a lever to lift interesting questions.

Questions, pertaining the overall _sense_ of what we're doing. About the meaning of the measurement we take (or lack of it). On "deep" engineering problems, then.

Engineering is the application of science to the solution of practical everyday problems. It's both a science, and a people thing. It has the power of changing the world for the better, but also to make it more miserable, and this may happen both because of what we know of the Nature (the physics, mathematics and biology side) and the nature of the problem itself, and the consequences its solution may have (the human side).

A good engineer is an empathetic creature, able to stay in touch with the feelings and the needs of their "clients" (and users, voluntary and not, human and not); and, should remain curious, willing to get deeper in understanding how things do work and behave. And, sure, should also be a good technician, a translator of necessities into devices, software code, and other things.

