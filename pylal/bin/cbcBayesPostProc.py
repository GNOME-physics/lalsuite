#!/usr/bin/env python

# Demonstrates use of the pylal.bayespputils module for producing stats/plots based on results of
# parameter estimation codes.
# This version print a summary file with means and variances
import sys
import os

from math import ceil,floor
import cPickle as pickle

from optparse import OptionParser
from ConfigParser import ConfigParser
from time import strftime

import numpy as np
from numpy import array,exp,cos,sin,arcsin,arccos,sqrt,size,mean,column_stack

import matplotlib
matplotlib.use("Agg")
from matplotlib import pyplot as plt

from pylal import SimInspiralUtils
from pylal import bayespputils as bppu
from pylal import git_version



__author__="Ben Aylott <benjamin.aylott@ligo.org>, John Veitch <john.veitch@ligo.org>"
__version__= "git id %s"%git_version.id
__date__= git_version.date


def pickle_to_file(obj,fname):
    """
    Pickle/serialize 'obj' into 'fname'.
    """
    filed=open(fname,'w')
    pickle.dump(obj,filed)
    filed.close()
#

def oneD_dict_to_file(dict,fname):
    filed=open(fname,'w')
    for key,value in dict.items():
        filed.write("%s %s\n"%(str(key),str(value)) )

def cbcBayesPostProc(outdir,data,oneDMenu,twoDGreedyMenu,GreedyRes,confidence_levels,twoDplots,injfile=None,eventnum=None,skyres=None,bayesfactornoise=None,bayesfactorcoherent=None):
    """
    This is a demonstration script for using the functionality/data structures
    contained in pylal.bayespputils . It will produce a webpage from a file containing
    posterior samples generated by the parameter estimation codes with 1D/2D plots
    and stats from the marginal posteriors for each parameter/set of parameters.
    """

    if eventnum is not None and injfile is None:
        print "You specified an event number but no injection file. Ignoring!"

    if data is None:
        print 'You must specify an input data file'
        exit(1)
    #
    if outdir is None:
        print "You must specify an output directory."
        exit(1)

    if not os.path.isdir(outdir):
        os.makedirs(outdir)
    #

    commonOutputFileObj=open(data[0])

    #Select injections using tc +/- 0.1s if it exists or eventnum from the injection file
    injection=None
    if injfile:
        import itertools
        injections = SimInspiralUtils.ReadSimInspiralFromFiles([injfile])
        if(eventnum is not None):
            if(len(injections)<eventnum):
                print "Error: You asked for event %d, but %s contains only %d injections" %(eventnum,injfile,len(injections))
                sys.exit(1)
            else:
                injection=injections[eventnum]
        else:
            if(len(injections)<1):
                print 'Warning: Cannot find injection with end time %f' %(means[2])
            else:
                injection = itertools.ifilter(lambda a: abs(a.get_end() - means[2]) < 0.1, injections).next()


    ## Load Bayes factors ##
    # Add Bayes factor information to summary file #
    if bayesfactornoise is not None:
        bfile=open(bayesfactornoise,'r')
        BSN=bfile.read()
        bfile.close()
        print 'BSN: %s'%BSN
    if bayesfactorcoherent is not None:
        bfile=open(bayesfactorcoherent,'r')
        BCI=bfile.read()
        bfile.close()
        print 'BCI: %s'%BCI

    #Create an instance of the posterior class using the posterior values loaded
    #from the file and any injection information (if given).
    pos = bppu.Posterior(commonOutputFileObj,SimInspiralTableEntry=injection)

    if ('mc' in pos.names or 'mchirp' in pos.names) and \
    'eta' in pos.names and \
    ('mass1' not in pos.names or 'm1' not in pos.names) and\
    ('m2' not in pos.names or 'm2' not in pos.names):

        if 'mc' in pos.names:
            mchirp_name='mc'
        else:
            mchirp_name='mchirp'

        inj_mass1=None
        inj_mass2=None
        if injection:
            inj_mass1,inj_mass2=bppu.mc2ms(injection.mchirp,injection.eta)

        mass1_samps,mass2_samps=bppu.mc2ms(pos[mchirp_name].samples,pos['eta'].samples)
        mass1_pos=bppu.OneDPosterior('m1',mass1_samps,injected_value=inj_mass1)
        mass2_pos=bppu.OneDPosterior('m2',mass2_samps,injected_value=inj_mass2)

        pos.append(mass1_pos)
        pos.append(mass2_pos)
            
    # print means, variances, and bayes factor in a summary file. The order of parameters is then one given here
    pars=['mchirp', 'eta','time','phi0','dist','RA','dec','psi','iota','m1','m2']
    summary_path=os.path.join(str(outdir),'summary.ini')
    summary_file=open(str(summary_path),'w')
    data_path=(str(data)[2:-2])
    data_array=np.loadtxt(str(data_path),skiprows=1)
    for i in pars:    
        summary_file.write('mean_'+str(i) +'\t'+'stdev_'+str(i)+'\t')
    summary_file.write('BSN \t BCI \n')
    for i in pars:
        if not (i=='m1' or i=='m2'):
            I=pars.index(i)
            summary_file.write(str(np.mean(data_array[:,I]))+'\t')
            summary_file.write(str(sqrt(np.var(data_array[:,I])))+'\t')
        elif i=='m1':
            summary_file.write(str(np.mean(mass1_samps))+'\t')
            summary_file.write(str(sqrt(np.var(mass1_samps)))+'\t')
        elif i=='m2':
            summary_file.write(str(np.mean(mass2_samps))+'\t')
            summary_file.write(str(sqrt(np.var(mass2_samps)))+'\t')

    if bayesfactornoise is not None:
        summary_file.write(str(BSN)+'\t')
    if bayesfactorcoherent is not None:
        summary_file.write(str(BCI)+'\t')
    summary_file.write('\n')       
    summary_file.close()

    ##Print some summary stats for the user...##
    #Number of samples
    print "Number of posterior samples: %i"%len(pos)
    # Means
    print 'Means:'
    print str(pos.means)
    #Median
    print 'Median:'
    print str(pos.medians)
    #maxL
    print 'maxL:'
    max_pos,max_pos_co=pos.maxL
    print max_pos_co

    #==================================================================#
    #Create web page
    #==================================================================#

    html=bppu.htmlPage('Posterior PDFs')

    #Create a section for meta-data/run information
    html_meta=html.add_section('Summary')
    html_meta.p('Produced from '+str(len(pos))+' posterior samples.')
    html_meta.p('Samples read from %s'%(data[0]))

    #Create a section for model selection results (if they exist)
    if bayesfactornoise is not None:
        html_model=html.add_section('Model selection')
        html_model.p('log Bayes factor ( coherent vs gaussian noise) = %s, Bayes factor=%f'%(BSN,exp(float(BSN))))
        if bayesfactorcoherent is not None:
            html_model.p('log Bayes factor ( coherent vs incoherent OR noise ) = %s, Bayes factor=%f'%(BCI,exp(float(BCI))))

    #Create a section for summary statistics
    html_stats=html.add_section('Summary statistics')
    html_stats.write(str(pos))
    

    #==================================================================#
    #Generate sky map
    #==================================================================#
    #If sky resolution parameter has been specified try and create sky map...
    skyreses=None
    sky_injection_cl=None
    if skyres is not None and 'ra' in pos.names and 'dec' in pos.names:
        #Greedy bin sky samples (ra,dec) into a grid on the sky which preserves
        #?
        top_ranked_sky_pixels,sky_injection_cl,skyreses,injection_area=bppu.greedy_bin_sky(pos,skyres,confidence_levels)
        print "BCI for sky area:"
        print skyreses
        #Create sky map in outdir
        bppu.plot_sky_map(top_ranked_sky_pixels,outdir)

    #Create a web page section for sky localization results/plots
    html_sky=html.add_section('Sky Localization')
    if injection:
        if sky_injection_cl:
            html_sky.p('Injection found at confidence interval %f in sky location'%(sky_injection_cl))
        else:
            html_sky.p('Injection not found in posterior bins in sky location!')
    html_sky.write('<img width="35%" src="skymap.png"/>')
    if skyres is not None:
        html_sky_write='<table border="1"><tr><th>Confidence region</th><th>size (sq. deg)</th></tr>'

        fracs=skyreses.keys()
        fracs.sort()

        skysizes=[skyreses[frac] for frac in fracs]
        for frac,skysize in zip(fracs,skysizes):
            html_sky_write+=('<tr><td>%f</td><td>%f</td></tr>'%(frac,skysize))
        html_sky_write+=('</table>')

        html_sky.write(html_sky_write)


    #==================================================================#
    #2D posteriors
    #==================================================================#

    #Loop over parameter pairs in twoDGreedyMenu and bin the sample pairs
    #using a greedy algorithm . The ranked pixels (toppoints) are used
    #to plot 2D histograms and evaluate Bayesian confidence intervals.

    #Make a folder for the 2D kde plots
    margdir=os.path.join(outdir,'2Dkde')
    if not os.path.isdir(margdir):
        os.makedirs(margdir)

    twobinsdir=os.path.join(outdir,'2Dbins')
    if not os.path.isdir(twobinsdir):
        os.makedirs(twobinsdir)

    #Add a section to the webpage for a table of the confidence interval
    #results.
    html_tcig=html.add_section('2D confidence intervals (greedy binning)')
    #Generate the top part of the table
    html_tcig_write='<table width="100%" border="1"><tr><th/>'
    confidence_levels.sort()
    for cl in confidence_levels:
        html_tcig_write+='<th>%f</th>'%cl
    if injection:
        html_tcig_write+='<th>Injection Confidence Level</th>'
        html_tcig_write+='<th>Injection Confidence Interval</th>'
    html_tcig_write+='</tr>'

    #=  Add a section for a table of 2D marginal PDFs (kde)
    html_tcmp=html.add_section('2D Marginal PDFs')
    html_tcmp.br()
    #Table matter
    html_tcmp_write='<table border="1" width="100%">'

    row_count=0
    for par1_name,par2_name in twoDGreedyMenu:
        par1_name=par1_name.lower()
        par2_name=par2_name.lower()
        print "Binning %s-%s to determine confidence levels ..."%(par1_name,par2_name)
        try:
            pos[par1_name.lower()]
        except KeyError:
            print "No input chain for %s, skipping binning."%par1_name
            continue
        try:
            pos[par2_name.lower()]
        except KeyError:
            print "No input chain for %s, skipping binning."%par2_name
            continue
        #Bin sizes
        try:
            par1_bin=GreedyRes[par1_name]
        except KeyError:
            print "Bin size is not set for %s, skipping %s/%s binning."%(par1_name,par1_name,par2_name)
            continue
        try:
            par2_bin=GreedyRes[par2_name]
        except KeyError:
            print "Bin size is not set for %s, skipping %s/%s binning."%(par2_name,par1_name,par2_name)
            continue

        #Form greedy binning input structure
        greedy2Params={par1_name:par1_bin,par2_name:par2_bin}

        #Greedy bin the posterior samples
        toppoints,injection_cl,reses,injection_area=\
        bppu.greedy_bin_two_param(pos,greedy2Params,confidence_levels)

        print "BCI %s-%s:"%(par1_name,par2_name)
        print reses

        #Generate new BCI html table row
        BCItableline='<tr><td>%s-%s</td>'%(par1_name,par2_name)
        cls=reses.keys()
        cls.sort()

        for cl in cls:
            BCItableline+='<td>%f</td>'%reses[cl]

        if injection is not None and injection_cl is not None:
            BCItableline+='<td>%f</td>'%injection_cl
            BCItableline+='<td>%f</td>'%injection_area
        BCItableline+='</tr>'

        #Append new table line to section html
        html_tcig_write+=BCItableline


        #= Plot 2D histograms of greedily binned points =#
        #greedy2PlotFig=bppu.plot_two_param_greedy_bins(np.array(toppoints),pos,greedy2Params)
        #greedy2PlotFig.savefig(os.path.join(twobinsdir,'%s-%s_greedy2.png'%(par1_name,par2_name)))

        #= Generate 2D kde plots =#
        print 'Generating %s-%s plot'%(par1_name,par2_name)

        par1_pos=pos[par1_name].samples
        par2_pos=pos[par2_name].samples

        if (size(np.unique(par1_pos))<2 or size(np.unique(par2_pos))<2):
            continue

        plot2DkdeParams={par1_name:50,par2_name:50}
        myfig=bppu.plot_two_param_kde(pos,plot2DkdeParams)

        figname=par1_name+'-'+par2_name+'_2Dkernel.png'
        twoDKdePath=os.path.join(margdir,figname)

        if row_count==0:
            html_tcmp_write+='<tr>'
        html_tcmp_write+='<td width="30%"><img width="100%" src="2Dkde/'+figname+'"/></td>'
        row_count+=1
        if row_count==3:
            html_tcmp_write+='</tr>'
            row_count=0

        myfig.savefig(twoDKdePath)


    #Finish off the BCI table and write it into the etree
    html_tcig_write+='</table>'
    html_tcig.write(html_tcig_write)

    #Finish off the 2D kde plot table
    while row_count!=0:
        html_tcmp_write+='<td/>'
        row_count+=1
        if row_count==3:
            row_count=0
            html_tcmp_write+='</tr>'
    html_tcmp_write+='</table>'
    html_tcmp.write(html_tcmp_write)
    #Add a link to all plots
    html_tcmp.br()
    html_tcmp.a("2Dkde/",'All 2D marginal PDFs (kde)')
    html_tcmp.hr()

    #==================================================================#
    #1D posteriors
    #==================================================================#

    #Loop over each parameter and determine the contigious and greedy
    #confidence levels and some statistics.

    #Add section for 1D confidence intervals
    html_ogci=html.add_section('1D confidence intervals (greedy binning)')
    #Generate the top part of the table
    html_ogci_write='<table width="100%" border="1"><tr><th/>'
    confidence_levels.sort()
    for cl in confidence_levels:
        html_ogci_write+='<th>%f</th>'%cl
    if injection:
        html_ogci_write+='<th>Injection Confidence Level</th>'
        html_ogci_write+='<th>Injection Confidence Interval</th>'
    html_ogci_write+='</tr>'

    #Add section for 1D marginal PDFs and sample plots
    html_ompdf=html.add_section('1D marginal posterior PDFs')
    html_ompdf.br()
    #Table matter
    html_ompdf_write= '<table><tr><th>Histogram and Kernel Density Estimate</th><th>Samples used</th></tr>'

    onepdfdir=os.path.join(outdir,'1Dpdf')
    if not os.path.isdir(onepdfdir):
        os.makedirs(onepdfdir)

    sampsdir=os.path.join(outdir,'1Dsamps')
    if not os.path.isdir(sampsdir):
        os.makedirs(sampsdir)

    for par_name in oneDMenu:
        par_name=par_name.lower()
        print "Binning %s to determine confidence levels ..."%par_name
        try:
            pos[par_name.lower()]
        except KeyError:
            print "No input chain for %s, skipping binning."%par_name
            continue
        try:
            par_bin=GreedyRes[par_name]
        except KeyError:
            print "Bin size is not set for %s, skipping binning."%par_name
            continue

        binParams={par_name:par_bin}

        toppoints,injectionconfidence,reses,injection_area=bppu.greedy_bin_one_param(pos,binParams,confidence_levels)

        oneDContCL,oneDContInj = bppu.contigious_interval_one_param(pos,binParams,confidence_levels)

        #Generate new BCI html table row
        BCItableline='<tr><td>%s</td>'%(par_name)
        cls=reses.keys()
        cls.sort()

        for cl in cls:
            BCItableline+='<td>%f</td>'%reses[cl]

        if injection is not None and injectionconfidence is not None and injection_area is not None:
            BCItableline+='<td>%f</td>'%injectionconfidence
            BCItableline+='<td>%f</td>'%injection_area
        BCItableline+='</tr>'

        #Append new table line to section html
        html_ogci_write+=BCItableline

        #Generate 1D histogram/kde plots
        print "Generating 1D plot for %s."%par_name
        oneDPDFParams={par_name:50}
        rbins,plotFig=bppu.plot_one_param_pdf(pos,oneDPDFParams)

        figname=par_name+'.png'
        oneDplotPath=os.path.join(onepdfdir,figname)
        plotFig.savefig(oneDplotPath)

        if rbins:
            print "r of injected value of %s (bins) = %f"%(par_name, rbins)

        ##Produce plot of raw samples
        myfig=plt.figure(figsize=(4,3.5),dpi=80)
        pos_samps=pos[par_name].samples
        plt.plot(pos_samps,'.',figure=myfig)
        injpar=pos[par_name].injval

        if injpar:
            if min(pos_samps)<injpar and max(pos_samps)>injpar:
                plt.plot([0,len(pos_samps)],[injpar,injpar],'r-.')
        myfig.savefig(os.path.join(sampsdir,figname.replace('.png','_samps.png')))

        html_ompdf_write+='<tr><td><img src="1Dpdf/'+figname+'"/></td><td><img src="1Dsamps/'+figname.replace('.png','_samps.png')+'"/></td></tr>'


    html_ompdf_write+='</table>'

    html_ompdf.write(html_ompdf_write)

    html_ogci_write+='</table>'
    html_ogci.write(html_ogci_write)

    html_ogci.hr()
    html_ogci.br()

    html_ompdf.hr()
    html_ompdf.br()

    html_footer=html.add_section('')
    html_footer.p('Produced using cbcBayesPostProc.py at '+strftime("%Y-%m-%d %H:%M:%S")+' .')
    html_footer.p(git_version.verbose_msg)

    #Save results page
    resultspage=open(os.path.join(outdir,'posplots.html'),'w')
    resultspage.write(str(html))

    # Save posterior samples too...
    posfilename=os.path.join(outdir,'posterior_samples.dat')
    posfile=open(posfilename,'w')
    input_file=open(data[0])
    posfile.write(input_file.read())
    #
    posfilename2=os.path.join(outdir,'posterior_samples2.dat')
    pos.write_to_file(posfilename2)

    #Close files
    input_file.close()
    posfile.close()
    resultspage.close()

if __name__=='__main__':

    parser=OptionParser()
    parser.add_option("-o","--outpath", dest="outpath",help="make page and plots in DIR", metavar="DIR")
    parser.add_option("-d","--data",dest="data",action="append",help="datafile")
    parser.add_option("-i","--inj",dest="injfile",help="SimInsipral injection file",metavar="INJ.XML",default=None)
    parser.add_option("--skyres",dest="skyres",help="Sky resolution to use to calculate sky box size",default=None)
    parser.add_option("--eventnum",dest="eventnum",action="store",default=None,help="event number in SimInspiral file of this signal",type="int",metavar="NUM")
    parser.add_option("--bsn",action="store",default=None,help="Optional file containing the bayes factor signal against noise",type="string")
    parser.add_option("--bci",action="store",default=None,help="Optional file containing the bayes factor coherent against incoherent models",type="string")

    (opts,args)=parser.parse_args()

    #List of parameters to plot/bin . Need to match (converted) column names.
    oneDMenu=['mtotal','m1','m2','mchirp','mc','distance','distMPC','dist','iota','psi','eta','ra','dec','a1','a2','phi1','theta1','phi2','theta2']
    #List of parameter pairs to bin . Need to match (converted) column names.
    twoDGreedyMenu=[['mc','eta'],['mchirp','eta'],['m1','m2'],['mtotal','eta'],['distance','iota'],['dist','iota'],['dist','m1'],['ra','dec']]
    #Bin size/resolution for binning. Need to match (converted) column names.
    greedyBinSizes={'mc':0.025,'m1':0.1,'m2':0.1,'mass1':0.1,'mass2':0.1,'mtotal':0.1,'eta':0.001,'iota':0.01,'time':1e-4,'distance':1.0,'dist':1.0,'mchirp':0.025,'a1':0.02,'a2':0.02,'phi1':0.05,'phi2':0.05,'theta1':0.05,'theta2':0.05,'ra':0.05,'dec':0.05}
    #Confidence levels
    confidenceLevels=[0.67,0.9,0.95,0.99]
    #2D plots list
    twoDplots=[['mc','eta'],['mchirp','eta'],['m1','m2'],['mtotal','eta'],['distance','iota'],['dist','iota'],['RA','dec'],['m1','dist'],['m2','dist'],['psi','iota'],['psi','distance'],['psi','dist'],['psi','phi0']]


    cbcBayesPostProc(opts.outpath,opts.data,oneDMenu,twoDGreedyMenu,greedyBinSizes,confidenceLevels,twoDplots,injfile=opts.injfile,eventnum=opts.eventnum,skyres=opts.skyres,bayesfactornoise=opts.bsn,bayesfactorcoherent=opts.bci)
#
