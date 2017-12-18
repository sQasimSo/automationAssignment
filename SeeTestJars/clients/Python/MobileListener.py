'''
Created on 2 Feb 2015

@author: user
'''
import abc

class MobileListener(object):
    '''
    classdocs
    '''


    def init(self, params):
        '''
        Constructor
     
        '''
    def setClient(self, client_):
        self.client = client_
        
    __metaclass__ = abc.ABCMeta
    @abc.abstractmethod
    def recover(self, elementType, xpath):
        '''
        '''